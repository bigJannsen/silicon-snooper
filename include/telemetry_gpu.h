#ifndef TELEMETRY_GPU_H
#define TELEMETRY_GPU_H

#include <stdint.h>
#include <time.h>
#include "output_mode.h"

typedef struct {
    int available;
    double utilization_percent;
    const char *source_key;
} GpuUtilization;

typedef struct {
    int available;
    double temperature_celsius;
    int pressure_level;
} GpuThermal;

typedef struct {
    uint64_t monotonic_ns;
    struct timespec wall_time;
    GpuUtilization utilization;
    GpuThermal thermal;
} GpuSnapshot;

int gpu_collect_snapshot(GpuSnapshot *snapshot);
void gpu_print_table(const GpuSnapshot *snapshot);
void gpu_print_json(const GpuSnapshot *snapshot, OutputFormat format);
int run_gpu_watch(int interval_ms, OutputFormat format);

#endif
