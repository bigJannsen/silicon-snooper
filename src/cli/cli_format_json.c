#include "cli_format_json.h"
#include <stdio.h>

static void emit_snapshot_json(const SnooperSnapshot *snapshot) {
    if (!snapshot) return;

    printf("{");
    printf("\"timestamp\":{\"wall\":\"%ld.%09ld\",\"monotonic_ns\":%llu}",
           (long)snapshot->wall_time.tv_sec,
           (long)snapshot->wall_time.tv_nsec,
           (unsigned long long)snapshot->monotonic_ns);
    printf(",\"cpu\":{\"used_percent\":%.2f}", snapshot->cpu_used_percent);
    printf(",\"gpu\":{\"available\":%s,\"used_percent\":%.2f}",
           snapshot->gpu_available ? "true" : "false",
           snapshot->gpu_available ? snapshot->gpu_used_percent : 0.0);

    if (snapshot->has_system_info) {
        printf(",\"system\":{\"model\":\"%s\",\"arch\":\"%s\",\"physical_cores\":%d,\"logical_cores\":%d,\"board_id\":\"%s\",\"product\":\"%s\",\"serial\":\"%s\",\"hardware_uuid\":\"%s\"}"
               , snapshot->system_info.cpu_model
               , snapshot->system_info.cpu_architecture
               , snapshot->system_info.physical_cores
               , snapshot->system_info.logical_cores
               , snapshot->system_info.board_id
               , snapshot->system_info.product_name
               , snapshot->system_info.serial_number
               , snapshot->system_info.hardware_uuid);
    }

    printf("}\n");
}

void cli_print_json(const SnooperSnapshot *snapshot, CliFormat format) {
    if (!snapshot) return;

    emit_snapshot_json(snapshot);

    if (format == CLI_FORMAT_JSON) {
        // do nothing extra
    }
}
