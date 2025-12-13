#include "telemetry_cpu.h"
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/processor_info.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint64_t timespec_to_ns(const struct timespec *ts) {
    return ((uint64_t)ts->tv_sec * 1000000000ULL) + (uint64_t)ts->tv_nsec;
}

static int capture_timestamps(uint64_t *monotonic_ns, struct timespec *wall) {
    if (!monotonic_ns || !wall) {
        return -1;
    }

    struct timespec mono = {0};
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &mono) != 0) {
        if (clock_gettime(CLOCK_MONOTONIC, &mono) != 0) {
            return -1;
        }
    }

    if (clock_gettime(CLOCK_REALTIME, wall) != 0) {
        return -1;
    }

    *monotonic_ns = timespec_to_ns(&mono);
    return 0;
}

static void free_processor_info(processor_info_array_t info, mach_msg_type_number_t count) {
    if (info != NULL && count > 0) {
        vm_deallocate(mach_task_self(), (vm_address_t)info, (vm_size_t)count * sizeof(integer_t));
    }
}

int cpu_sample_collect(CpuSample *sample) {
    if (sample == NULL) {
        return -1;
    }

    memset(sample, 0, sizeof(*sample));

    if (capture_timestamps(&sample->monotonic_ns, &sample->wall_time) != 0) {
        return -1;
    }

    natural_t cpu_count = 0;
    processor_info_array_t cpu_info = NULL;
    mach_msg_type_number_t info_count = 0;

    kern_return_t kr = host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &cpu_count, &cpu_info, &info_count);
    if (kr != KERN_SUCCESS) {
        return -1;
    }

    if (cpu_count == 0 || cpu_info == NULL) {
        free_processor_info(cpu_info, info_count);
        return -1;
    }

    sample->cores = calloc(cpu_count, sizeof(CpuTicks));
    if (sample->cores == NULL) {
        free_processor_info(cpu_info, info_count);
        return -1;
    }

    for (natural_t i = 0; i < cpu_count; ++i) {
        processor_cpu_load_info_t cpu_load = (processor_cpu_load_info_t)(cpu_info + (i * CPU_STATE_MAX));
        sample->cores[i].user = (uint64_t)cpu_load[CPU_STATE_USER];
        sample->cores[i].system = (uint64_t)cpu_load[CPU_STATE_SYSTEM];
        sample->cores[i].idle = (uint64_t)cpu_load[CPU_STATE_IDLE];
        sample->cores[i].nice = (uint64_t)cpu_load[CPU_STATE_NICE];
    }

    sample->core_count = cpu_count;
    free_processor_info(cpu_info, info_count);
    return 0;
}

void cpu_sample_destroy(CpuSample *sample) {
    if (sample == NULL) {
        return;
    }

    free(sample->cores);
    sample->cores = NULL;
    sample->core_count = 0;
}

static void compute_usage(const CpuTicks *prev, const CpuTicks *curr, CpuUsage *usage) {
    uint64_t user_delta = curr->user >= prev->user ? curr->user - prev->user : 0;
    uint64_t system_delta = curr->system >= prev->system ? curr->system - prev->system : 0;
    uint64_t idle_delta = curr->idle >= prev->idle ? curr->idle - prev->idle : 0;
    uint64_t nice_delta = curr->nice >= prev->nice ? curr->nice - prev->nice : 0;

    uint64_t total = user_delta + system_delta + idle_delta + nice_delta;
    if (total == 0) {
        usage->user = 0.0;
        usage->system = 0.0;
        usage->idle = 0.0;
        return;
    }

    usage->user = (double)user_delta * 100.0 / (double)total;
    usage->system = (double)system_delta * 100.0 / (double)total;
    usage->idle = (double)idle_delta * 100.0 / (double)total;
}

int cpu_usage_from_delta(const CpuSample *previous, const CpuSample *current, CpuUsageReport *report) {
    if (previous == NULL || current == NULL || report == NULL) {
        return -1;
    }

    if (previous->core_count == 0 || current->core_count == 0) {
        return -1;
    }

    size_t cores = previous->core_count < current->core_count ? previous->core_count : current->core_count;
    memset(report, 0, sizeof(*report));

    report->cores = calloc(cores, sizeof(CpuUsage));
    if (report->cores == NULL) {
        return -1;
    }

    CpuTicks aggregate_prev = {0};
    CpuTicks aggregate_curr = {0};

    for (size_t i = 0; i < cores; ++i) {
        compute_usage(&previous->cores[i], &current->cores[i], &report->cores[i]);

        aggregate_prev.user += previous->cores[i].user;
        aggregate_prev.system += previous->cores[i].system;
        aggregate_prev.idle += previous->cores[i].idle;
        aggregate_prev.nice += previous->cores[i].nice;

        aggregate_curr.user += current->cores[i].user;
        aggregate_curr.system += current->cores[i].system;
        aggregate_curr.idle += current->cores[i].idle;
        aggregate_curr.nice += current->cores[i].nice;
    }

    compute_usage(&aggregate_prev, &aggregate_curr, &report->overall);

    report->core_count = cores;
    report->monotonic_ns = current->monotonic_ns;
    report->wall_time = current->wall_time;

    return 0;
}

void cpu_usage_report_destroy(CpuUsageReport *report) {
    if (report == NULL) {
        return;
    }

    free(report->cores);
    report->cores = NULL;
    report->core_count = 0;
}

static void format_time(const struct timespec *ts, char *buffer, size_t size) {
    if (buffer == NULL || size == 0) {
        return;
    }

    struct tm local_tm;
    if (localtime_r(&ts->tv_sec, &local_tm) == NULL) {
        snprintf(buffer, size, "<invalid>");
        return;
    }

    char date_buf[32];
    if (strftime(date_buf, sizeof(date_buf), "%Y-%m-%dT%H:%M:%S", &local_tm) == 0) {
        snprintf(buffer, size, "<invalid>");
        return;
    }

    long millis = ts->tv_nsec / 1000000L;
    snprintf(buffer, size, "%s.%03ld", date_buf, millis);
}

void cpu_usage_print_table(const CpuUsageReport *report) {
    if (report == NULL) {
        return;
    }

    char wall_buf[64];
    format_time(&report->wall_time, wall_buf, sizeof(wall_buf));
    double monotonic_sec = (double)report->monotonic_ns / 1e9;

    printf("Time: %s | monotonic: %.3fs\n", wall_buf, monotonic_sec);
    printf("Overall   | User: %6.2f%% | System: %6.2f%% | Idle: %6.2f%%\n",
           report->overall.user, report->overall.system, report->overall.idle);
    printf("-----------------------------------------------\n");
    for (size_t i = 0; i < report->core_count; ++i) {
        printf("Core %2zu | User: %6.2f%% | System: %6.2f%% | Idle: %6.2f%%\n",
               i, report->cores[i].user, report->cores[i].system, report->cores[i].idle);
    }
    printf("\n");
}

void cpu_usage_print_json(const CpuUsageReport *report, OutputFormat format) {
    if (report == NULL) {
        return;
    }

    (void)format;

    char wall_buf[64];
    format_time(&report->wall_time, wall_buf, sizeof(wall_buf));
    double monotonic_sec = (double)report->monotonic_ns / 1e9;

    printf("{");
    printf("\"timestamp\":{\"wall\":\"%s\",\"monotonic_seconds\":%.3f}", wall_buf, monotonic_sec);
    printf(",\"overall\":{\"user\":%.2f,\"system\":%.2f,\"idle\":%.2f}",
           report->overall.user, report->overall.system, report->overall.idle);
    printf(",\"cores\":[");
    for (size_t i = 0; i < report->core_count; ++i) {
        printf("{\"id\":%zu,\"user\":%.2f,\"system\":%.2f,\"idle\":%.2f}",
               i, report->cores[i].user, report->cores[i].system, report->cores[i].idle);
        if (i + 1 < report->core_count) {
            printf(",");
        }
    }
    printf("]}");

    printf("\n");
}

static void sleep_for_interval(int interval_ms) {
    if (interval_ms <= 0) {
        return;
    }

    struct timespec req;
    req.tv_sec = interval_ms / 1000;
    req.tv_nsec = (long)(interval_ms % 1000) * 1000000L;
    nanosleep(&req, NULL);
}

int run_cpu_watch(int interval_ms, OutputFormat format) {
    if (interval_ms <= 0) {
        fprintf(stderr, "Interval must be greater than zero.\n");
        return -1;
    }

    CpuSample previous = {0};
    CpuSample current = {0};
    CpuUsageReport report = {0};

    if (cpu_sample_collect(&previous) != 0) {
        fprintf(stderr, "Failed to collect initial CPU sample.\n");
        return -1;
    }

    while (1) {
        sleep_for_interval(interval_ms);

        if (cpu_sample_collect(&current) != 0) {
            fprintf(stderr, "Failed to collect CPU sample.\n");
            cpu_sample_destroy(&previous);
            return -1;
        }

        if (cpu_usage_from_delta(&previous, &current, &report) != 0) {
            fprintf(stderr, "Failed to compute CPU usage.\n");
            cpu_sample_destroy(&previous);
            cpu_sample_destroy(&current);
            return -1;
        }

        if (format == OUTPUT_TABLE) {
            cpu_usage_print_table(&report);
        } else {
            cpu_usage_print_json(&report, format);
        }

        cpu_usage_report_destroy(&report);
        cpu_sample_destroy(&previous);
        previous = current;
        memset(&current, 0, sizeof(current));
    }

    return 0;
}

