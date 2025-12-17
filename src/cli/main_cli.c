#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "cli_args.h"
#include "cli_format_table.h"
#include "cli_format_json.h"
#include "snooper/telemetry.h"
#include "snooper/system_info.h"

static void sleep_for_interval(int interval_ms) {
    if (interval_ms <= 0) return;
    struct timespec req;
    req.tv_sec = interval_ms / 1000;
    req.tv_nsec = (long)(interval_ms % 1000) * 1000000L;
    nanosleep(&req, NULL);
}

static int handle_info(int show_identifiers) {
    SnooperSystemInfo info;
    if (snooper_system_info_read(&info, show_identifiers) != SNOOPER_OK) {
        fprintf(stderr, "Failed to read system info.\n");
        return 1;
    }
    cli_print_system_info(&info);
    return 0;
}

static int run_watch(const CliOptions *opts) {
    SnooperTelemetry telemetry;
    if (snooper_telemetry_init(&telemetry, opts->show_identifiers) != SNOOPER_OK) {
        fprintf(stderr, "Failed to initialize telemetry.\n");
        return 1;
    }

    int printed_header = 0;

    for (;;) {
        SnooperSnapshot snapshot;
        SnooperStatus rc = snooper_snapshot_collect(&telemetry, &snapshot);
        if (rc == SNOOPER_ERR_WARMUP) {
            sleep_for_interval(opts->interval_ms);
            continue;
        } else if (rc != SNOOPER_OK) {
            fprintf(stderr, "Failed to collect snapshot (%d).\n", rc);
            snooper_telemetry_destroy(&telemetry);
            return 1;
        }

        if (opts->format == CLI_FORMAT_TABLE) {
            cli_print_table(&snapshot, !printed_header);
            printed_header = 1;
        } else {
            cli_print_json(&snapshot, opts->format);
        }

        sleep_for_interval(opts->interval_ms);
    }

    snooper_telemetry_destroy(&telemetry);
    return 0;
}

int main(int argc, char **argv) {
    CliOptions opts;
    if (cli_parse_arguments(argc, argv, &opts) != 0) {
        cli_print_usage(argv[0]);
        return 1;
    }

    if (opts.command == CLI_CMD_INFO) {
        return handle_info(opts.show_identifiers);
    }

    return run_watch(&opts);
}
