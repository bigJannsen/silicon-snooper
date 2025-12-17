#ifndef SNOOPER_CLI_FORMAT_TABLE_H
#define SNOOPER_CLI_FORMAT_TABLE_H

#include "snooper/telemetry.h"

void cli_print_table(const SnooperSnapshot *snapshot, int print_header);
void cli_print_system_info(const SnooperSystemInfo *info);

#endif
