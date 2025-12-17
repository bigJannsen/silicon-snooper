#include "cli_format_table.h"
#include <stdio.h>
#include <time.h>

static void format_time(const struct timespec *ts, char *buffer, size_t size) {
    if (!ts || !buffer || size == 0) return;

    struct tm local_tm;
    if (localtime_r(&ts->tv_sec, &local_tm) == NULL) {
        snprintf(buffer, size, "<invalid>");
        return;
    }

    char date_buf[32];
    if (strftime(date_buf, sizeof(date_buf), "%Y-%m-%dT%H:%M:%S", &local_tm) == 0) {
        snprintf(buffer, size, "<invalid>");
        return;
    }

    long millis = ts->tv_nsec / 1000000L;
    snprintf(buffer, size, "%s.%03ld", date_buf, millis);
}

void cli_print_system_info(const SnooperSystemInfo *info) {
    if (!info) return;

    printf("CPU Model       : %s\n", info->cpu_model);
    printf("Architecture    : %s\n", info->cpu_architecture);
    printf("Physical Cores  : %d\n", info->physical_cores);
    printf("Logical Cores   : %d\n", info->logical_cores);
    printf("Board ID        : %s\n", info->board_id);
    printf("Product Name    : %s\n", info->product_name);
    printf("Serial Number   : %s\n", info->serial_number);
    printf("Hardware UUID   : %s\n", info->hardware_uuid);
}

void cli_print_table(const SnooperSnapshot *snapshot, int print_header) {
    if (!snapshot) return;

    if (print_header && snapshot->has_system_info) {
        cli_print_system_info(&snapshot->system_info);
        printf("----------------------------------------\n");
    }

    char wall_buf[64];
    format_time(&snapshot->wall_time, wall_buf, sizeof(wall_buf));
    double monotonic_sec = (double)snapshot->monotonic_ns / 1e9;

    printf("Time: %s | monotonic: %.3fs\n", wall_buf, monotonic_sec);
    printf("CPU Used: %6.2f%%\n", snapshot->cpu_used_percent);

    if (snapshot->gpu_available) {
        printf("GPU Used: %6.2f%%\n", snapshot->gpu_used_percent);
    } else {
        printf("GPU Used: N/A\n");
    }
    printf("\n");
}
