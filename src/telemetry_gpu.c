#include "telemetry_gpu.h"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <stdio.h>
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

static int read_cfnumber_double(CFNumberRef number, double *out_value) {
    if (number == NULL || out_value == NULL) {
        return -1;
    }

    if (!CFNumberGetValue(number, kCFNumberDoubleType, out_value)) {
        int64_t temp = 0;
        if (!CFNumberGetValue(number, kCFNumberSInt64Type, &temp)) {
            return -1;
        }
        *out_value = (double)temp;
    }
    return 0;
}

static int extract_utilization(CFDictionaryRef stats, GpuUtilization *util) {
    const CFStringRef keys[] = {
        CFSTR("GPU Busy"),
        CFSTR("Device Utilization %"),
        CFSTR("HW Utilization"),
        CFSTR("Renderer Utilization %"),
        CFSTR("Tiler Utilization %")
    };

    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
        CFNumberRef val = (CFNumberRef)CFDictionaryGetValue(stats, keys[i]);
        if (val != NULL && CFGetTypeID(val) == CFNumberGetTypeID()) {
            double percent = 0.0;
            if (read_cfnumber_double(val, &percent) == 0) {
                util->available = 1;
                util->utilization_percent = percent;
                const char *label = CFStringGetCStringPtr(keys[i], kCFStringEncodingUTF8);
                util->source_key = label ? label : "PerformanceStatistics";
                return 0;
            }
        }
    }

    util->available = 0;
    util->utilization_percent = 0.0;
    util->source_key = NULL;
    return -1;
}

static int extract_thermal(CFDictionaryRef stats, GpuThermal *thermal) {
    CFNumberRef temp = (CFNumberRef)CFDictionaryGetValue(stats, CFSTR("Temperature"));
    if (temp && CFGetTypeID(temp) == CFNumberGetTypeID()) {
        double value = 0.0;
        if (read_cfnumber_double(temp, &value) == 0) {
            thermal->available = 1;
            thermal->temperature_celsius = value;
        }
    }

    CFNumberRef pressure = (CFNumberRef)CFDictionaryGetValue(stats, CFSTR("ThermalLevel"));
    if (pressure && CFGetTypeID(pressure) == CFNumberGetTypeID()) {
        double level = 0.0;
        if (read_cfnumber_double(pressure, &level) == 0) {
            thermal->pressure_level = (int)level;
            thermal->available = 1;
        }
    }

    return thermal->available ? 0 : -1;
}

int gpu_collect_snapshot(GpuSnapshot *snapshot) {
    if (snapshot == NULL) {
        return -1;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->thermal.temperature_celsius = -1.0;
    snapshot->thermal.pressure_level = -1;

    if (capture_timestamps(&snapshot->monotonic_ns, &snapshot->wall_time) != 0) {
        return -1;
    }

    io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("IOAccelerator"));
    if (!service) {
        snapshot->utilization.available = 0;
        snapshot->thermal.available = 0;
        return 0;
    }

    CFDictionaryRef stats = IORegistryEntryCreateCFProperty(service, CFSTR("PerformanceStatistics"), kCFAllocatorDefault, 0);
    if (stats && CFGetTypeID(stats) == CFDictionaryGetTypeID()) {
        extract_utilization(stats, &snapshot->utilization);
        extract_thermal(stats, &snapshot->thermal);
        CFRelease(stats);
    }

    IOObjectRelease(service);
    return 0;
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

void gpu_print_table(const GpuSnapshot *snapshot) {
    if (snapshot == NULL) {
        return;
    }

    char wall_buf[64];
    format_time(&snapshot->wall_time, wall_buf, sizeof(wall_buf));
    double monotonic_sec = (double)snapshot->monotonic_ns / 1e9;

    printf("Time: %s | monotonic: %.3fs\n", wall_buf, monotonic_sec);

    if (snapshot->utilization.available) {
        const char *source = snapshot->utilization.source_key ? snapshot->utilization.source_key : "PerformanceStatistics";
        printf("GPU Utilization: %6.2f%% (%s)\n", snapshot->utilization.utilization_percent, source);
    } else {
        printf("GPU Utilization: <unavailable>\n");
    }

    if (snapshot->thermal.available) {
        if (snapshot->thermal.temperature_celsius >= 0.0) {
            printf("GPU Temperature: %5.1f C\n", snapshot->thermal.temperature_celsius);
        }
        if (snapshot->thermal.pressure_level >= 0) {
            printf("Thermal Level : %d\n", snapshot->thermal.pressure_level);
        }
    } else {
        printf("Thermal/Pressure: <unavailable>\n");
    }

    printf("\n");
}

void gpu_print_json(const GpuSnapshot *snapshot, OutputFormat format) {
    if (snapshot == NULL) {
        return;
    }

    (void)format;

    char wall_buf[64];
    format_time(&snapshot->wall_time, wall_buf, sizeof(wall_buf));
    double monotonic_sec = (double)snapshot->monotonic_ns / 1e9;

    const char *source = snapshot->utilization.source_key ? snapshot->utilization.source_key : "PerformanceStatistics";
    printf("{");
    printf("\"timestamp\":{\"wall\":\"%s\",\"monotonic_seconds\":%.3f}", wall_buf, monotonic_sec);
    if (snapshot->utilization.available) {
        printf(",\"gpu\":{\"available\":true,\"utilization_percent\":%.2f,\"source\":\"%s\"}",
               snapshot->utilization.utilization_percent,
               source);
    } else {
        printf(",\"gpu\":{\"available\":false,\"utilization_percent\":0.00,\"source\":null}");
    }

    printf(",\"thermal\":{\"available\":%s,\"temperature_c\":",
           snapshot->thermal.available ? "true" : "false");
    if (snapshot->thermal.available && snapshot->thermal.temperature_celsius >= 0.0) {
        printf("%.2f", snapshot->thermal.temperature_celsius);
    } else {
        printf("null");
    }

    printf(",\"pressure_level\":");
    if (snapshot->thermal.available && snapshot->thermal.pressure_level >= 0) {
        printf("%d", snapshot->thermal.pressure_level);
    } else {
        printf("null");
    }

    printf("}}\n");
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

int run_gpu_watch(int interval_ms, OutputFormat format) {
    if (interval_ms <= 0) {
        fprintf(stderr, "Interval must be greater than zero.\n");
        return -1;
    }

    while (1) {
        GpuSnapshot snapshot;
        if (gpu_collect_snapshot(&snapshot) != 0) {
            fprintf(stderr, "Failed to collect GPU telemetry.\n");
            return -1;
        }

        if (format == OUTPUT_TABLE) {
            gpu_print_table(&snapshot);
        } else {
            gpu_print_json(&snapshot, format);
        }

        sleep_for_interval(interval_ms);
    }

    return 0;
}

