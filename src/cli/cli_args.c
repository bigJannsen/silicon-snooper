#include "cli_args.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void cli_print_usage(const char *progname) {
    printf("Usage:\n");
    printf("  %s cpu --watch <milliseconds> [--json | --ndjson] [--show-identifiers]\n", progname);
    printf("  %s gpu --watch <milliseconds> [--json | --ndjson] [--show-identifiers]\n", progname);
    printf("  %s info [--show-identifiers]\n", progname);
    printf("\nOptions:\n");
    printf("  --watch <ms>         Sampling interval in milliseconds (required for cpu/gpu).\n");
    printf("  --json               Emit JSON output.\n");
    printf("  --ndjson             Emit newline-delimited JSON per sample.\n");
    printf("  --show-identifiers   Reveal serial number and hardware UUID.\n");
    printf("  -h, --help           Show this help message.\n");
}

int cli_parse_arguments(int argc, char **argv, CliOptions *out) {
    if (!out) return -1;

    if (argc < 2) {
        return -1;
    }

    if (strcmp(argv[1], "cpu") == 0) {
        out->command = CLI_CMD_CPU;
    } else if (strcmp(argv[1], "gpu") == 0) {
        out->command = CLI_CMD_GPU;
    } else if (strcmp(argv[1], "info") == 0) {
        out->command = CLI_CMD_INFO;
    } else if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        return -1;
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        return -1;
    }

    out->interval_ms = 0;
    out->format = CLI_FORMAT_TABLE;
    out->show_identifiers = 0;

    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--watch") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for --watch.\n");
                return -1;
            }
            out->interval_ms = atoi(argv[++i]);
            if (out->interval_ms <= 0) {
                fprintf(stderr, "Interval must be positive.\n");
                return -1;
            }
        } else if (strcmp(argv[i], "--json") == 0) {
            out->format = CLI_FORMAT_JSON;
        } else if (strcmp(argv[i], "--ndjson") == 0) {
            out->format = CLI_FORMAT_NDJSON;
        } else if (strcmp(argv[i], "--show-identifiers") == 0) {
            out->show_identifiers = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            return -1;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return -1;
        }
    }

    if ((out->command == CLI_CMD_CPU || out->command == CLI_CMD_GPU) && out->interval_ms <= 0) {
        fprintf(stderr, "--watch <milliseconds> is required for cpu/gpu.\n");
        return -1;
    }

    return 0;
}
