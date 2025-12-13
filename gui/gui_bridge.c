#include "gui_bridge.h"
#include <string.h>

int gui_cpu_probe_init(GuiCpuProbe *probe) {
    if (!probe) {
        return -1;
    }
    memset(probe, 0, sizeof(*probe));
    return 0;
}

int gui_cpu_probe_sample(GuiCpuProbe *probe, CpuUsage *overall_usage) {
    if (!probe || !overall_usage) {
        return -1;
    }

    CpuSample current = {0};
    if (cpu_sample_collect(&current) != 0) {
        return -1;
    }

    if (!probe->has_previous) {
        probe->previous = current;
        probe->has_previous = 1;
        return 1; // indicates first sample, no delta yet
    }

    CpuUsageReport report = {0};
    int rc = cpu_usage_from_delta(&probe->previous, &current, &report);
    cpu_sample_destroy(&probe->previous);
    probe->previous = current;

    if (rc != 0) {
        return -1;
    }

    *overall_usage = report.overall;
    cpu_usage_report_destroy(&report);
    return 0;
}

void gui_cpu_probe_destroy(GuiCpuProbe *probe) {
    if (!probe) {
        return;
    }

    if (probe->has_previous) {
        cpu_sample_destroy(&probe->previous);
        probe->has_previous = 0;
    }
}
