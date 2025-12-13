#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "telemetry_cpu.h"
#include "telemetry_gpu.h"
#include "cpu.h"

static void print_usage(const char *progname) {
    printf("Usage:\n");
    printf("  %s cpu --watch <milliseconds> [--json | --ndjson]\n", progname);
    printf("  %s gpu --watch <milliseconds> [--json | --ndjson]\n", progname);
    printf("\nOptions:\n");
    printf("  --watch <ms>   Sampling interval in milliseconds (required).\n");
    printf("  --json         Emit JSON output.\n");
    printf("  --ndjson       Emit newline-delimited JSON per sample.\n");
    printf("  -h, --help     Show this help message.\n");
}

static int parse_arguments(int argc, char **argv, int *interval_ms, OutputFormat *format) {
    *interval_ms = 0;
    *format = OUTPUT_TABLE;

    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--watch") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for --watch.\n");
                return -1;
            }
            *interval_ms = atoi(argv[++i]);
            if (*interval_ms <= 0) {
                fprintf(stderr, "Interval must be a positive integer.\n");
                return -1;
            }
        } else if (strcmp(argv[i], "--json") == 0) {
            *format = OUTPUT_JSON;
        } else if (strcmp(argv[i], "--ndjson") == 0) {
            *format = OUTPUT_NDJSON;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return -1;
        }
    }

    if (*interval_ms <= 0) {
        fprintf(stderr, "--watch <milliseconds> is required.\n");
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    const char *command = argv[1];
    int interval_ms = 0;
    OutputFormat format = OUTPUT_TABLE;

    if (strcmp(command, "cpu") == 0 || strcmp(command, "gpu") == 0) {
        if (parse_arguments(argc, argv, &interval_ms, &format) != 0) {
            return 1;
        }

        if (strcmp(command, "cpu") == 0) {
            if (run_cpu_watch(interval_ms, format) != 0) {
                return 1;
            }
            return 0;
        }

        if (run_gpu_watch(interval_ms, format) != 0) {
            return 1;
        }
        return 0;
    }

    // Default: show basic CPU information for compatibility
    CpuInfo cpu;
    if (get_cpu_info(&cpu) != 0) {
        fprintf(stderr, "Failed to read CPU info.\n");
        return 1;
    }

    printf("CPU Model       : %s\n", cpu.model);
    printf("Architecture    : %s\n", cpu.architecture);
    printf("Physical Cores  : %d\n", cpu.physical_cores);
    printf("Logical Cores   : %d\n", cpu.logical_cores);
    printf("\nUnknown command '%s'. Use --help for usage.\n", command);
    return 1;
}
