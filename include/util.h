#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdint.h>

// Sicheres Kopieren von Strings (Null-terminiert, kein Overflow)
void safe_strcpy(char *dst, size_t dst_size, const char *src);

// sysctl-Leser
int read_sysctl_string(const char *name, char *buffer, size_t buffer_size);
int read_sysctl_int(const char *name, int *value);
int read_sysctl_uint64(const char *name, uint64_t *value);

#endif
