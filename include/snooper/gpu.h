#ifndef SNOOPER_GPU_H
#define SNOOPER_GPU_H

#include <stdint.h>
#include <time.h>
#include "snooper/errors.h"

typedef struct {
    int available;
    double utilization_percent;
    uint64_t monotonic_ns;
    struct timespec wall_time;
} SnooperGpuSample;

typedef struct {
    int initialized;
} GpuProbe;

SnooperStatus gpu_probe_init(GpuProbe *probe);
void gpu_probe_destroy(GpuProbe *probe);
SnooperStatus gpu_probe_sample(GpuProbe *probe, SnooperGpuSample *sample);

#endif
