#ifndef SNOOPER_TELEMETRY_H
#define SNOOPER_TELEMETRY_H

#include <stdint.h>
#include <time.h>
#include "snooper/cpu.h"
#include "snooper/gpu.h"
#include "snooper/system_info.h"

typedef struct {
    uint64_t monotonic_ns;
    struct timespec wall_time;
    double cpu_used_percent;
    double gpu_used_percent;
    int gpu_available;
    SnooperSystemInfo system_info;
    int has_system_info;
} SnooperSnapshot;

typedef struct {
    CpuProbe cpu_probe;
    GpuProbe gpu_probe;
    SnooperSystemInfo system_info;
    int reveal_identifiers;
    int system_info_loaded;
} SnooperTelemetry;

SnooperStatus snooper_telemetry_init(SnooperTelemetry *telemetry, int reveal_identifiers);
void snooper_telemetry_destroy(SnooperTelemetry *telemetry);
SnooperStatus snooper_snapshot_collect(SnooperTelemetry *telemetry, SnooperSnapshot *out);

#endif
