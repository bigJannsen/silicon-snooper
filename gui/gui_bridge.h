#ifndef GUI_BRIDGE_H
#define GUI_BRIDGE_H

#include "telemetry_cpu.h"

typedef struct {
    CpuSample previous;
    int has_previous;
} GuiCpuProbe;

int gui_cpu_probe_init(GuiCpuProbe *probe);
int gui_cpu_probe_sample(GuiCpuProbe *probe, CpuUsage *overall_usage);
void gui_cpu_probe_destroy(GuiCpuProbe *probe);

#endif
