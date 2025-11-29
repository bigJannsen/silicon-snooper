#include "../include/cpu.h"
#include "../include/util.h"
#include <string.h>


int get_cpu_info(CpuInfo *info) {
    memset(info, 0, sizeof(*info));

    if (read_sysctl_string("machdep.cpu.brand_string",
        info->model, sizeof(info->model)) != 0) {
        return -1;
    }

    int arm64 = 0;

    if (read_sysctl_int("hw.optional.arm64", &arm64) == 0 && arm64 == 1) {
        safe_strcpy(info->architecture, sizeof(info->architecture), "ARM64");
    } else {
        read_sysctl_string("hw.machine", info->architecture, sizeof(info->architecture));
    }

    if (read_sysctl_int("hw.physicalcpu", &info->physical_cores) != 0) return -1;
    if (read_sysctl_int("hw.logicalcpu", &info->logical_cores) != 0) return -1;

    return 0;
}

