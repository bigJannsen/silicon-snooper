#include "snooper/cpu.h"
#include "timeutil.h"
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <mach/processor_info.h>
#include <stdlib.h>
#include <string.h>

static SnooperStatus cpu_sample_collect(SnooperCpuSample *sample) {
    if (!sample) {
        return SNOOPER_ERR_INVALID;
    }

    memset(sample, 0, sizeof(*sample));

    if (snooper_capture_timestamps(&sample->monotonic_ns, &sample->wall_time) != SNOOPER_OK) {
        return SNOOPER_ERR_UNAVAILABLE;
    }

    natural_t cpu_count = 0;
    processor_info_array_t cpu_info = NULL;
    mach_msg_type_number_t info_count = 0;

    kern_return_t kr = host_processor_info(mach_host_self(), PROCESSOR_CPU_LOAD_INFO, &cpu_count, &cpu_info, &info_count);
    if (kr != KERN_SUCCESS || cpu_count == 0 || cpu_info == NULL) {
        return SNOOPER_ERR_UNAVAILABLE;
    }

    sample->cores = calloc(cpu_count, sizeof(SnooperCpuTicks));
    if (!sample->cores) {
        vm_deallocate(mach_task_self(), (vm_address_t)cpu_info, (vm_size_t)info_count * sizeof(integer_t));
        return SNOOPER_ERR_NOMEM;
    }

    for (natural_t i = 0; i < cpu_count; ++i) {
        processor_cpu_load_info_t cpu_load = (processor_cpu_load_info_t)(cpu_info + (i * CPU_STATE_MAX));
        sample->cores[i].user = (uint64_t)cpu_load->cpu_ticks[CPU_STATE_USER];
        sample->cores[i].system = (uint64_t)cpu_load->cpu_ticks[CPU_STATE_SYSTEM];
        sample->cores[i].idle = (uint64_t)cpu_load->cpu_ticks[CPU_STATE_IDLE];
        sample->cores[i].nice = (uint64_t)cpu_load->cpu_ticks[CPU_STATE_NICE];
    }

    sample->core_count = cpu_count;
    vm_deallocate(mach_task_self(), (vm_address_t)cpu_info, (vm_size_t)info_count * sizeof(integer_t));
    return SNOOPER_OK;
}

static void cpu_sample_destroy(SnooperCpuSample *sample) {
    if (!sample) return;
    free(sample->cores);
    sample->cores = NULL;
    sample->core_count = 0;
}

static void compute_usage(const SnooperCpuTicks *prev, const SnooperCpuTicks *curr, SnooperCpuUsage *usage) {
    uint64_t user_delta = curr->user >= prev->user ? curr->user - prev->user : 0;
    uint64_t system_delta = curr->system >= prev->system ? curr->system - prev->system : 0;
    uint64_t idle_delta = curr->idle >= prev->idle ? curr->idle - prev->idle : 0;
    uint64_t nice_delta = curr->nice >= prev->nice ? curr->nice - prev->nice : 0;

    uint64_t total = user_delta + system_delta + idle_delta + nice_delta;
    if (total == 0) {
        usage->user = usage->system = usage->idle = 0.0;
        return;
    }

    usage->user = (double)user_delta * 100.0 / (double)total;
    usage->system = (double)system_delta * 100.0 / (double)total;
    usage->idle = (double)idle_delta * 100.0 / (double)total;
}

static SnooperStatus cpu_usage_from_delta(const SnooperCpuSample *previous, const SnooperCpuSample *current, SnooperCpuUsageReport *report) {
    if (!previous || !current || !report) {
        return SNOOPER_ERR_INVALID;
    }

    if (previous->core_count == 0 || current->core_count == 0) {
        return SNOOPER_ERR_UNAVAILABLE;
    }

    size_t cores = previous->core_count < current->core_count ? previous->core_count : current->core_count;
    memset(report, 0, sizeof(*report));

    report->per_core = calloc(cores, sizeof(SnooperCpuUsage));
    if (!report->per_core) {
        return SNOOPER_ERR_NOMEM;
    }

    SnooperCpuTicks aggregate_prev = {0};
    SnooperCpuTicks aggregate_curr = {0};

    for (size_t i = 0; i < cores; ++i) {
        compute_usage(&previous->cores[i], &current->cores[i], &report->per_core[i]);

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

    return SNOOPER_OK;
}

SnooperStatus cpu_probe_init(CpuProbe *probe) {
    if (!probe) {
        return SNOOPER_ERR_INVALID;
    }
    memset(probe, 0, sizeof(*probe));
    return SNOOPER_OK;
}

void cpu_probe_destroy(CpuProbe *probe) {
    if (!probe) return;
    if (probe->has_previous) {
        cpu_sample_destroy(&probe->previous);
        probe->has_previous = 0;
    }
}

void cpu_usage_report_destroy(SnooperCpuUsageReport *report) {
    if (!report) return;
    free(report->per_core);
    report->per_core = NULL;
    report->core_count = 0;
}

SnooperStatus cpu_probe_sample(CpuProbe *probe, SnooperCpuUsageReport *report) {
    if (!probe || !report) {
        return SNOOPER_ERR_INVALID;
    }

    SnooperCpuSample current = {0};
    SnooperStatus status = cpu_sample_collect(&current);
    if (status != SNOOPER_OK) {
        return status;
    }

    if (!probe->has_previous) {
        probe->previous = current;
        probe->has_previous = 1;
        return SNOOPER_ERR_WARMUP;
    }

    status = cpu_usage_from_delta(&probe->previous, &current, report);
    cpu_sample_destroy(&probe->previous);
    probe->previous = current;

    return status;
}
