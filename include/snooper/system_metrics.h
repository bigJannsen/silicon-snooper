#ifndef SNOOPER_SYSTEM_METRICS_H
#define SNOOPER_SYSTEM_METRICS_H

#include <stdint.h>
#include "snooper/errors.h"

typedef struct {
    uint64_t memory_used_bytes;
    uint64_t memory_free_bytes;
    uint64_t memory_compressed_bytes;
    double load_avg_1;
    double load_avg_5;
    double load_avg_15;
    uint64_t uptime_seconds;
    int process_count;
    int thread_count;
    int has_memory;
    int has_load;
    int has_uptime;
    int has_process_info;
} SnooperSystemMetrics;

SnooperStatus snooper_system_metrics_read(SnooperSystemMetrics *metrics);

#endif
