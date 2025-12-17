#ifndef SNOOPER_CLI_FORMAT_JSON_H
#define SNOOPER_CLI_FORMAT_JSON_H

#include "snooper/telemetry.h"
#include "cli_args.h"

void cli_print_json(const SnooperSnapshot *snapshot, CliFormat format);

#endif
