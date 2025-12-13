#ifndef GUI_TELEMETRY_BRIDGE_H
#define GUI_TELEMETRY_BRIDGE_H

#include <stddef.h>
#include "telemetry_cpu.h"
#include "telemetry_gpu.h"
#include "cpu.h"
#include "deviceinfo.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    CpuSample previous;
    int has_previous;
} GuiCpuProbe;

typedef struct {
    int available;
    double utilization_percent;
} GuiGpuSample;

typedef struct {
    char cpu_model[128];
    char cpu_architecture[32];
    int physical_cores;
    int logical_cores;
    char board_id[128];
    char product_name[128];
    char serial_number[128];
    char hardware_uuid[128];
} GuiSystemInfo;

int gui_cpu_probe_init(GuiCpuProbe *probe);
int gui_cpu_probe_sample(GuiCpuProbe *probe, CpuUsage *overall_usage);
void gui_cpu_probe_destroy(GuiCpuProbe *probe);

int gui_gpu_probe_sample(GuiGpuSample *sample);

int gui_get_system_info(GuiSystemInfo *info, int show_identifiers);

#ifdef __cplusplus
}
#endif

#endif
