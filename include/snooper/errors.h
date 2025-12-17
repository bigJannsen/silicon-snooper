#ifndef SNOOPER_ERRORS_H
#define SNOOPER_ERRORS_H

typedef enum {
    SNOOPER_OK = 0,
    SNOOPER_ERR_INVALID = -1,
    SNOOPER_ERR_UNAVAILABLE = -2,
    SNOOPER_ERR_NOMEM = -3,
    SNOOPER_ERR_WARMUP = 1
} SnooperStatus;

#endif
