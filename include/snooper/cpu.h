#ifndef SNOOPER_CPU_H
#define SNOOPER_CPU_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include "snooper/errors.h"

typedef struct {
    uint64_t user;
    uint64_t system;
    uint64_t idle;
    uint64_t nice;
} SnooperCpuTicks;

typedef struct {
    uint64_t monotonic_ns;
    struct timespec wall_time;
    SnooperCpuTicks *cores;
    size_t core_count;
} SnooperCpuSample;

typedef struct {
    double user;
    double system;
    double idle;
} SnooperCpuUsage;

typedef struct {
    SnooperCpuUsage overall;
    SnooperCpuUsage *per_core;
    size_t core_count;
    uint64_t monotonic_ns;
    struct timespec wall_time;
} SnooperCpuUsageReport;

typedef struct {
    SnooperCpuSample previous;
    int has_previous;
} CpuProbe;

SnooperStatus cpu_probe_init(CpuProbe *probe);
void cpu_probe_destroy(CpuProbe *probe);
SnooperStatus cpu_probe_sample(CpuProbe *probe, SnooperCpuUsageReport *report);
void cpu_usage_report_destroy(SnooperCpuUsageReport *report);

#endif
