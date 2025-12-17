#ifndef SNOOPER_TIMEUTIL_H
#define SNOOPER_TIMEUTIL_H

#include <stdint.h>
#include <time.h>
#include "snooper/errors.h"

uint64_t snooper_timespec_to_ns(const struct timespec *ts);
SnooperStatus snooper_capture_timestamps(uint64_t *monotonic_ns, struct timespec *wall);

#endif
