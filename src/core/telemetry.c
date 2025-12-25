#include "snooper/telemetry.h"
#include "snooper/errors.h"

SnooperStatus snooper_telemetry_init(SnooperTelemetry *telemetry, int reveal_identifiers) {
    if (!telemetry) return SNOOPER_ERR_INVALID;
    memset(telemetry, 0, sizeof(*telemetry));

    telemetry->reveal_identifiers = reveal_identifiers ? 1 : 0;

    SnooperStatus status = cpu_probe_init(&telemetry->cpu_probe);
    if (status != SNOOPER_OK) return status;

    status = gpu_probe_init(&telemetry->gpu_probe);
    if (status != SNOOPER_OK) return status;

    if (snooper_system_info_read(&telemetry->system_info, telemetry->reveal_identifiers) == SNOOPER_OK) {
        telemetry->system_info_loaded = 1;
    }

    return SNOOPER_OK;
}

void snooper_telemetry_destroy(SnooperTelemetry *telemetry) {
    if (!telemetry) return;
    cpu_probe_destroy(&telemetry->cpu_probe);
    gpu_probe_destroy(&telemetry->gpu_probe);
    telemetry->system_info_loaded = 0;
}

SnooperStatus snooper_snapshot_collect(SnooperTelemetry *telemetry, SnooperSnapshot *out) {
    if (!telemetry || !out) return SNOOPER_ERR_INVALID;

    memset(out, 0, sizeof(*out));

    SnooperCpuUsageReport cpu_report = {0};
    SnooperStatus status = cpu_probe_sample(&telemetry->cpu_probe, &cpu_report);
    if (status != SNOOPER_OK) {
        if (status == SNOOPER_ERR_WARMUP) {
            cpu_usage_report_destroy(&cpu_report);
            return SNOOPER_ERR_WARMUP;
        }
        cpu_usage_report_destroy(&cpu_report);
        return status;
    }

    out->cpu_used_percent = 100.0 - cpu_report.overall.idle;
    out->monotonic_ns = cpu_report.monotonic_ns;
    out->wall_time = cpu_report.wall_time;

    SnooperGpuSample gpu_sample = {0};
    SnooperStatus gpu_status = gpu_probe_sample(&telemetry->gpu_probe, &gpu_sample);
    if (gpu_status == SNOOPER_OK) {
        out->gpu_available = gpu_sample.available;
        out->gpu_used_percent = gpu_sample.utilization_percent;
    } else {
        out->gpu_available = 0;
        out->gpu_used_percent = 0.0;
    }

    cpu_usage_report_destroy(&cpu_report);

    if (telemetry->system_info_loaded) {
        out->system_info = telemetry->system_info;
        out->has_system_info = 1;
    }

    (void)snooper_system_metrics_read(&out->system_metrics);

    return SNOOPER_OK;
}
