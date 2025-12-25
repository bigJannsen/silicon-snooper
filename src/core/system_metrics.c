#include "snooper/system_metrics.h"

#include <mach/mach.h>
#include <sys/sysctl.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

static int read_load_average(double *one, double *five, double *fifteen) {
    double loads[3] = {0.0, 0.0, 0.0};
    if (getloadavg(loads, 3) != 3) {
        return -1;
    }
    if (one) *one = loads[0];
    if (five) *five = loads[1];
    if (fifteen) *fifteen = loads[2];
    return 0;
}

static int read_uptime(uint64_t *seconds_out) {
    struct timeval boot_time = {0};
    size_t size = sizeof(boot_time);
    if (sysctlbyname("kern.boottime", &boot_time, &size, NULL, 0) != 0) {
        return -1;
    }
    time_t now = time(NULL);
    if (now < 0) {
        return -1;
    }
    if (seconds_out) {
        *seconds_out = (uint64_t)difftime(now, boot_time.tv_sec);
    }
    return 0;
}

static int read_memory_stats(uint64_t *used, uint64_t *free, uint64_t *compressed) {
    mach_port_t host = mach_host_self();
    vm_size_t page_size = 0;
    if (host_page_size(host, &page_size) != KERN_SUCCESS) {
        return -1;
    }

    vm_statistics64_data_t vm_stat = {0};
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    if (host_statistics64(host, HOST_VM_INFO64, (host_info64_t)&vm_stat, &count) != KERN_SUCCESS) {
        return -1;
    }

    uint64_t used_pages = (uint64_t)vm_stat.active_count
                          + (uint64_t)vm_stat.inactive_count
                          + (uint64_t)vm_stat.wire_count
                          + (uint64_t)vm_stat.speculative_count;
    uint64_t free_pages = (uint64_t)vm_stat.free_count;
    uint64_t compressed_pages = (uint64_t)vm_stat.compressor_page_count;

    if (used) *used = used_pages * (uint64_t)page_size;
    if (free) *free = free_pages * (uint64_t)page_size;
    if (compressed) *compressed = compressed_pages * (uint64_t)page_size;

    return 0;
}

static int read_process_count(int *count_out) {
    int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
    size_t size = 0;
    if (sysctl(mib, 4, NULL, &size, NULL, 0) != 0 || size == 0) {
        return -1;
    }

    void *buffer = malloc(size);
    if (!buffer) {
        return -1;
    }

    if (sysctl(mib, 4, buffer, &size, NULL, 0) != 0) {
        free(buffer);
        return -1;
    }

    int count = (int)(size / sizeof(struct kinfo_proc));
    free(buffer);
    if (count_out) *count_out = count;
    return 0;
}

SnooperStatus snooper_system_metrics_read(SnooperSystemMetrics *metrics) {
    if (!metrics) {
        return SNOOPER_ERR_INVALID;
    }

    memset(metrics, 0, sizeof(*metrics));
    metrics->process_count = -1;
    metrics->thread_count = -1;

    if (read_memory_stats(&metrics->memory_used_bytes,
                          &metrics->memory_free_bytes,
                          &metrics->memory_compressed_bytes) == 0) {
        metrics->has_memory = 1;
    }

    if (read_load_average(&metrics->load_avg_1,
                          &metrics->load_avg_5,
                          &metrics->load_avg_15) == 0) {
        metrics->has_load = 1;
    }

    if (read_uptime(&metrics->uptime_seconds) == 0) {
        metrics->has_uptime = 1;
    }

    if (read_process_count(&metrics->process_count) == 0) {
        metrics->has_process_info = 1;
    }

    return SNOOPER_OK;
}
