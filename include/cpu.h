#ifndef CPU_H
#define CPU_H

typedef struct {
    char model[128];
    char architecture[32];
    int physical_cores;
    int logical_cores;
} CpuInfo;

int get_cpu_info(CpuInfo *info);

#endif
