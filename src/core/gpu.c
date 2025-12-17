#include "snooper/gpu.h"
#include "timeutil.h"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <string.h>

static SnooperStatus read_cfnumber_double(CFNumberRef number, double *out_value) {
    if (!number || !out_value) {
        return SNOOPER_ERR_INVALID;
    }

    if (!CFNumberGetValue(number, kCFNumberDoubleType, out_value)) {
        int64_t temp = 0;
        if (!CFNumberGetValue(number, kCFNumberSInt64Type, &temp)) {
            return SNOOPER_ERR_UNAVAILABLE;
        }
        *out_value = (double)temp;
    }
    return SNOOPER_OK;
}

static SnooperStatus extract_utilization(CFDictionaryRef stats, SnooperGpuSample *sample) {
    const CFStringRef keys[] = {
        CFSTR("GPU Busy"),
        CFSTR("Device Utilization %"),
        CFSTR("HW Utilization"),
        CFSTR("Renderer Utilization %"),
        CFSTR("Tiler Utilization %")
    };

    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); ++i) {
        CFNumberRef val = (CFNumberRef)CFDictionaryGetValue(stats, keys[i]);
        if (val && CFGetTypeID(val) == CFNumberGetTypeID()) {
            double percent = 0.0;
            if (read_cfnumber_double(val, &percent) == SNOOPER_OK) {
                sample->available = 1;
                sample->utilization_percent = percent;
                return SNOOPER_OK;
            }
        }
    }

    sample->available = 0;
    sample->utilization_percent = 0.0;
    return SNOOPER_ERR_UNAVAILABLE;
}

SnooperStatus gpu_probe_init(GpuProbe *probe) {
    if (!probe) return SNOOPER_ERR_INVALID;
    memset(probe, 0, sizeof(*probe));
    probe->initialized = 1;
    return SNOOPER_OK;
}

void gpu_probe_destroy(GpuProbe *probe) {
    if (!probe) return;
    probe->initialized = 0;
}

SnooperStatus gpu_probe_sample(GpuProbe *probe, SnooperGpuSample *sample) {
    if (!probe || !sample || !probe->initialized) {
        return SNOOPER_ERR_INVALID;
    }

    memset(sample, 0, sizeof(*sample));

    if (snooper_capture_timestamps(&sample->monotonic_ns, &sample->wall_time) != SNOOPER_OK) {
        return SNOOPER_ERR_UNAVAILABLE;
    }

    io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault, IOServiceMatching("IOAccelerator"));
    if (!service) {
        sample->available = 0;
        return SNOOPER_OK;
    }

    CFDictionaryRef stats = IORegistryEntryCreateCFProperty(service, CFSTR("PerformanceStatistics"), kCFAllocatorDefault, 0);
    if (stats && CFGetTypeID(stats) == CFDictionaryGetTypeID()) {
        extract_utilization(stats, sample);
        CFRelease(stats);
    }

    IOObjectRelease(service);
    return SNOOPER_OK;
}
