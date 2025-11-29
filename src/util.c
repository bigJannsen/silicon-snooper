#include "util.h"
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/sysctl.h>

void safe_strcpy(char *dst, size_t dst_size, const char *src) {
    if (!dst || !src || dst_size == 0) return;
    snprintf(dst, dst_size, "%s", src);
}

int read_sysctl_string(const char *name, char *buffer, size_t buffer_size) {
    size_t size = buffer_size;
    // sysctlbyname - declaration in sys/sysctl.h - takes Key out of kernel database
    if (sysctlbyname(name, buffer, &size, NULL, 0) != 0) {
        return -1;
    }
    buffer[buffer_size - 1] = '\0';
    return 0;
}

int read_sysctl_int(const char *name, int *value) {
    size_t size = sizeof(*value);
    if (sysctlbyname(name, value, &size, NULL, 0) != 0) {
        return -1;
    }
    return 0;
}

int read_sysctl_uint64(const char *name, uint64_t *value) {
    size_t size = sizeof(*value);
    if (sysctlbyname(name, value, &size, NULL, 0) != 0) {
        return -1;
    }
    return 0;
}
