#ifndef GUI_BRIDGE_H
#define GUI_BRIDGE_H

#include "snooper/telemetry.h"

typedef struct {
    SnooperTelemetry telemetry;
} GuiTelemetry;

int gui_telemetry_init(GuiTelemetry *telemetry, int show_identifiers);
void gui_telemetry_destroy(GuiTelemetry *telemetry);
int gui_poll_snapshot(GuiTelemetry *telemetry, SnooperSnapshot *snapshot);
const SnooperSystemInfo *gui_system_info(const GuiTelemetry *telemetry);

#endif
