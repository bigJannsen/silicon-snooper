#include "gui_bridge.h"

int gui_telemetry_init(GuiTelemetry *telemetry, int show_identifiers) {
    if (!telemetry) return -1;
    SnooperStatus status = snooper_telemetry_init(&telemetry->telemetry, show_identifiers);
    return status == SNOOPER_OK ? 0 : -1;
}

void gui_telemetry_destroy(GuiTelemetry *telemetry) {
    if (!telemetry) return;
    snooper_telemetry_destroy(&telemetry->telemetry);
}

int gui_poll_snapshot(GuiTelemetry *telemetry, SnooperSnapshot *snapshot) {
    if (!telemetry || !snapshot) return -1;
    SnooperStatus status = snooper_snapshot_collect(&telemetry->telemetry, snapshot);
    if (status == SNOOPER_ERR_WARMUP) {
        return 1;
    }
    return status == SNOOPER_OK ? 0 : -1;
}

const SnooperSystemInfo *gui_system_info(const GuiTelemetry *telemetry) {
    if (!telemetry) return NULL;
    if (telemetry->telemetry.system_info_loaded) {
        return &telemetry->telemetry.system_info;
    }
    return NULL;
}
