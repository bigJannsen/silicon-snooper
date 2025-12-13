#include "gui_telemetry_bridge.h"
#include "util.h"
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

int gui_gpu_probe_sample(GuiGpuSample *sample) {
    if (!sample) {
        return -1;
    }

    GpuSnapshot snapshot;
    if (gpu_collect_snapshot(&snapshot) != 0) {
        return -1;
    }

    sample->available = snapshot.utilization.available;
    sample->utilization_percent = snapshot.utilization.utilization_percent;
    return 0;
}

static void mask_identifier(char *value) {
    if (!value) {
        return;
    }
    size_t len = strlen(value);
    if (len <= 4) {
        return;
    }
    for (size_t i = 0; i + 4 < len; ++i) {
        value[i] = '*';
    }
}

int gui_get_system_info(GuiSystemInfo *info, int show_identifiers) {
    if (!info) {
        return -1;
    }

    memset(info, 0, sizeof(*info));

    CpuInfo cpu = {0};
    if (get_cpu_info(&cpu) != 0) {
        return -1;
    }

    safe_strcpy(info->cpu_model, sizeof(info->cpu_model), cpu.model);
    safe_strcpy(info->cpu_architecture, sizeof(info->cpu_architecture), cpu.architecture);
    info->physical_cores = cpu.physical_cores;
    info->logical_cores = cpu.logical_cores;

    if (get_board_id(info->board_id, sizeof(info->board_id)) != 0) {
        safe_strcpy(info->board_id, sizeof(info->board_id), "<unavailable>");
    }
    if (get_product_name(info->product_name, sizeof(info->product_name)) != 0) {
        safe_strcpy(info->product_name, sizeof(info->product_name), "<unavailable>");
    }

    if (get_serial(info->serial_number, sizeof(info->serial_number)) != 0) {
        safe_strcpy(info->serial_number, sizeof(info->serial_number), "<unavailable>");
    }
    if (get_hardware_uuid(info->hardware_uuid, sizeof(info->hardware_uuid)) != 0) {
        safe_strcpy(info->hardware_uuid, sizeof(info->hardware_uuid), "<unavailable>");
    }

    if (!show_identifiers) {
        mask_identifier(info->serial_number);
        mask_identifier(info->hardware_uuid);
    }

    return 0;
}
