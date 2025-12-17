#ifndef SNOOPER_SYSTEM_INFO_H
#define SNOOPER_SYSTEM_INFO_H

#include <stddef.h>
#include "snooper/errors.h"

typedef struct {
    char cpu_model[128];
    char cpu_architecture[32];
    int physical_cores;
    int logical_cores;
    char board_id[128];
    char product_name[128];
    char serial_number[128];
    char hardware_uuid[128];
} SnooperSystemInfo;

SnooperStatus snooper_system_info_read(SnooperSystemInfo *info, int reveal_identifiers);

#endif
