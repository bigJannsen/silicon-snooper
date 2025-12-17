#include "timeutil.h"
#include <time.h>

uint64_t snooper_timespec_to_ns(const struct timespec *ts) {
    if (!ts) {
        return 0;
    }
    return ((uint64_t)ts->tv_sec * 1000000000ULL) + (uint64_t)ts->tv_nsec;
}

SnooperStatus snooper_capture_timestamps(uint64_t *monotonic_ns, struct timespec *wall) {
    if (!monotonic_ns || !wall) {
        return SNOOPER_ERR_INVALID;
    }

    struct timespec mono = {0};
    if (clock_gettime(CLOCK_MONOTONIC_RAW, &mono) != 0) {
        if (clock_gettime(CLOCK_MONOTONIC, &mono) != 0) {
            return SNOOPER_ERR_UNAVAILABLE;
        }
    }

    if (clock_gettime(CLOCK_REALTIME, wall) != 0) {
        return SNOOPER_ERR_UNAVAILABLE;
    }

    *monotonic_ns = snooper_timespec_to_ns(&mono);
    return SNOOPER_OK;
}
