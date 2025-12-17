#ifndef SNOOPER_CLI_ARGS_H
#define SNOOPER_CLI_ARGS_H

typedef enum {
    CLI_CMD_CPU,
    CLI_CMD_GPU,
    CLI_CMD_INFO
} CliCommand;

typedef enum {
    CLI_FORMAT_TABLE,
    CLI_FORMAT_JSON,
    CLI_FORMAT_NDJSON
} CliFormat;

typedef struct {
    CliCommand command;
    int interval_ms;
    CliFormat format;
    int show_identifiers;
} CliOptions;

int cli_parse_arguments(int argc, char **argv, CliOptions *out);
void cli_print_usage(const char *progname);

#endif
