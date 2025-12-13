#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_host.h>
#include <sys/sysctl.h>
#include <sys/time.h>

typedef struct {
    char model[128];
    char architecture[32];
    int physical_cores;
    int logical_cores;
} CpuInfo;

typedef struct {
    uint64_t total_bytes;
    uint64_t used_bytes;
} MemoryInfo;

typedef struct {
    char version[64];
    char build[64];
} OsInfo;

static int read_sysctl_string(const char *name, char *buffer, size_t buffer_size) {
    size_t size = buffer_size;
    if (sysctlbyname(name, buffer, &size, NULL, 0) != 0) {
        return -1;
    }
    if (size >= buffer_size) {
        buffer[buffer_size - 1] = '\0';
    } else {
        buffer[size] = '\0';
    }
    return 0;
}

static int read_sysctl_int(const char *name, int *value) {
    size_t size = sizeof(*value);
    if (sysctlbyname(name, value, &size, NULL, 0) != 0) {
        return -1;
    }
    return 0;
}

static int read_sysctl_uint64(const char *name, uint64_t *value) {
    size_t size = sizeof(*value);
    if (sysctlbyname(name, value, &size, NULL, 0) != 0) {
        return -1;
    }
    return 0;
}

static double bytes_to_gb(uint64_t bytes) {
    const double bytes_per_gb = 1024.0 * 1024.0 * 1024.0;
    return bytes / bytes_per_gb;
}

static int get_cpu_info(CpuInfo *info) {
    if (!info) {
        return -1;
    }
    memset(info, 0, sizeof(*info));

    if (read_sysctl_string("machdep.cpu.brand_string", info->model, sizeof(info->model)) != 0) {
        return -1;
    }

    int is_arm64 = 0;
    if (read_sysctl_int("hw.optional.arm64", &is_arm64) == 0 && is_arm64 == 1) {
        strncpy(info->architecture, "ARM64", sizeof(info->architecture) - 1);
    } else if (read_sysctl_string("hw.machine", info->architecture, sizeof(info->architecture)) != 0) {
        strncpy(info->architecture, "Unknown", sizeof(info->architecture) - 1);
    }

    if (read_sysctl_int("hw.physicalcpu", &info->physical_cores) != 0) {
        return -1;
    }
    if (read_sysctl_int("hw.logicalcpu", &info->logical_cores) != 0) {
        return -1;
    }

    return 0;
}

static int get_memory_info(MemoryInfo *info) {
    if (!info) {
        return -1;
    }
    memset(info, 0, sizeof(*info));

    if (read_sysctl_uint64("hw.memsize", &info->total_bytes) != 0) {
        return -1;
    }

    vm_size_t page_size = 0;
    if (host_page_size(mach_host_self(), &page_size) != KERN_SUCCESS) {
        return -1;
    }

    vm_statistics64_data_t vm_stat;
    mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
    kern_return_t kr = host_statistics64(mach_host_self(), HOST_VM_INFO64, (host_info64_t)&vm_stat, &count);
    if (kr != KERN_SUCCESS) {
        return -1;
    }

    uint64_t active = (uint64_t)vm_stat.active_count * page_size;
    uint64_t wired = (uint64_t)vm_stat.wire_count * page_size;
    uint64_t compressed = (uint64_t)vm_stat.compressor_page_count * page_size;
    info->used_bytes = active + wired + compressed;

    return 0;
}

static int get_os_info(OsInfo *info) {
    if (!info) {
        return -1;
    }
    memset(info, 0, sizeof(*info));

    if (read_sysctl_string("kern.osproductversion", info->version, sizeof(info->version)) != 0) {
        return -1;
    }

    if (read_sysctl_string("kern.osversion", info->build, sizeof(info->build)) != 0) {
        return -1;
    }

    return 0;
}

static int get_system_uptime(time_t *uptime_seconds) {
    if (!uptime_seconds) {
        return -1;
    }

    struct timeval boottime = {0};
    size_t size = sizeof(boottime);
    int mib[2] = {CTL_KERN, KERN_BOOTTIME};
    if (sysctl(mib, 2, &boottime, &size, NULL, 0) != 0) {
        return -1;
    }

    struct timeval current_time;
    if (gettimeofday(&current_time, NULL) != 0) {
        return -1;
    }

    *uptime_seconds = current_time.tv_sec - boottime.tv_sec;
    return 0;
}

static int get_load_average(double load[3]) {
    if (!load) {
        return -1;
    }

    if (getloadavg(load, 3) != 3) {
        return -1;
    }

    return 0;
}

static int get_hostname(char *buffer, size_t size) {
    if (!buffer || size == 0) {
        return -1;
    }

    if (gethostname(buffer, size) != 0) {
        return -1;
    }

    buffer[size - 1] = '\0';
    return 0;
}

static void print_uptime(time_t seconds) {
    const int seconds_per_minute = 60;
    const int seconds_per_hour = 3600;
    const int seconds_per_day = 86400;

    int days = (int)(seconds / seconds_per_day);
    seconds %= seconds_per_day;
    int hours = (int)(seconds / seconds_per_hour);
    seconds %= seconds_per_hour;
    int minutes = (int)(seconds / seconds_per_minute);
    int remaining_seconds = (int)(seconds % seconds_per_minute);

    printf("System Uptime     : %dd %dh %dm %ds\n", days, hours, minutes, remaining_seconds);
}

int main(void) {
    CpuInfo cpu_info;
    MemoryInfo memory_info;
    OsInfo os_info;
    double load[3] = {0};
    time_t uptime_seconds = 0;
    char hostname[256] = {0};

    if (get_cpu_info(&cpu_info) != 0) {
        fprintf(stderr, "Failed to read CPU information.\n");
        return EXIT_FAILURE;
    }

    if (get_memory_info(&memory_info) != 0) {
        fprintf(stderr, "Failed to read memory information.\n");
        return EXIT_FAILURE;
    }

    if (get_os_info(&os_info) != 0) {
        fprintf(stderr, "Failed to read OS information.\n");
        return EXIT_FAILURE;
    }

    if (get_system_uptime(&uptime_seconds) != 0) {
        fprintf(stderr, "Failed to read system uptime.\n");
        return EXIT_FAILURE;
    }

    if (get_load_average(load) != 0) {
        fprintf(stderr, "Failed to read system load.\n");
        return EXIT_FAILURE;
    }

    if (get_hostname(hostname, sizeof(hostname)) != 0) {
        fprintf(stderr, "Failed to read hostname.\n");
        return EXIT_FAILURE;
    }

    printf("\n=== Hardware Readout (macOS) ===\n");
    printf("CPU Model         : %s\n", cpu_info.model);
    printf("Architecture      : %s\n", cpu_info.architecture);
    printf("Cores (phys/log)  : %d / %d\n", cpu_info.physical_cores, cpu_info.logical_cores);
    printf("RAM Total         : %.2f GB\n", bytes_to_gb(memory_info.total_bytes));
    printf("RAM Used          : %.2f GB\n", bytes_to_gb(memory_info.used_bytes));
    printf("macOS Version     : %s\n", os_info.version);
    printf("macOS Build       : %s\n", os_info.build);
    print_uptime(uptime_seconds);
    printf("Load Average      : 1m %.2f | 5m %.2f | 15m %.2f\n", load[0], load[1], load[2]);
    printf("Hostname          : %s\n", hostname);

    return EXIT_SUCCESS;
}
#else
int main(void) {
    fprintf(stderr, "This program is intended for macOS.\n");
    return EXIT_FAILURE;
}
#endif
