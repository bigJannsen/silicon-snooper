#ifndef TELEMETRY_CPU_H
#define TELEMETRY_CPU_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include "output_mode.h"

typedef struct {
    uint64_t user;
    uint64_t system;
    uint64_t idle;
    uint64_t nice;
} CpuTicks;

typedef struct {
    uint64_t monotonic_ns;
    struct timespec wall_time;
    size_t core_count;
    CpuTicks *cores;
} CpuSample;

typedef struct {
    double user;
    double system;
    double idle;
} CpuUsage;

typedef struct {
    uint64_t monotonic_ns;
    struct timespec wall_time;
    size_t core_count;
    CpuUsage *cores;
    CpuUsage overall;
} CpuUsageReport;

int cpu_sample_collect(CpuSample *sample);
void cpu_sample_destroy(CpuSample *sample);
int cpu_usage_from_delta(const CpuSample *previous, const CpuSample *current, CpuUsageReport *report);
void cpu_usage_report_destroy(CpuUsageReport *report);
void cpu_usage_print_table(const CpuUsageReport *report);
void cpu_usage_print_json(const CpuUsageReport *report, OutputFormat format);
int run_cpu_watch(int interval_ms, OutputFormat format);

#endif
