#include "snooper/system_info.h"
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <stdio.h>
#include <string.h>
#include <sys/sysctl.h>

static void safe_strcpy(char *dst, size_t dst_size, const char *src) {
    if (!dst || !src || dst_size == 0) {
        return;
    }
    snprintf(dst, dst_size, "%s", src);
}

static SnooperStatus read_sysctl_string(const char *name, char *buffer, size_t buffer_size) {
    size_t size = buffer_size;
    if (sysctlbyname(name, buffer, &size, NULL, 0) != 0) {
        return SNOOPER_ERR_UNAVAILABLE;
    }
    buffer[buffer_size - 1] = '\0';
    return SNOOPER_OK;
}

static SnooperStatus read_sysctl_int(const char *name, int *value) {
    size_t size = sizeof(*value);
    if (sysctlbyname(name, value, &size, NULL, 0) != 0) {
        return SNOOPER_ERR_UNAVAILABLE;
    }
    return SNOOPER_OK;
}

static SnooperStatus get_cf_property(const char *path, const char *key, char *buf, size_t size) {
    if (!buf || size == 0) return SNOOPER_ERR_INVALID;

    io_registry_entry_t entry = IORegistryEntryFromPath(kIOMainPortDefault, path);
    if (!entry) return SNOOPER_ERR_UNAVAILABLE;

    CFStringRef cfkey = CFStringCreateWithCString(NULL, key, kCFStringEncodingUTF8);
    CFTypeRef prop = IORegistryEntryCreateCFProperty(entry, cfkey, kCFAllocatorDefault, 0);

    CFRelease(cfkey);
    IOObjectRelease(entry);

    if (!prop) return SNOOPER_ERR_UNAVAILABLE;

    SnooperStatus status = SNOOPER_ERR_UNAVAILABLE;

    if (CFGetTypeID(prop) == CFStringGetTypeID()) {
        if (CFStringGetCString((CFStringRef)prop, buf, size, kCFStringEncodingUTF8)) {
            status = SNOOPER_OK;
        }
    } else if (CFGetTypeID(prop) == CFDataGetTypeID()) {
        CFDataRef data = (CFDataRef)prop;
        CFIndex len = CFDataGetLength(data);
        const UInt8 *bytes = CFDataGetBytePtr(data);

        int is_text = 1;
        for (CFIndex i = 0; i < len; i++) {
            if (bytes[i] == 0) break;
            if (bytes[i] < 0x20 || bytes[i] > 0x7E) {
                is_text = 0;
                break;
            }
        }

        if (is_text) {
            snprintf(buf, size, "%s", bytes);
        } else {
            size_t out = 0;
            for (CFIndex i = 0; i < len && out + 3 < size; i++)
                out += snprintf(buf + out, size - out, "%02X", bytes[i]);
            buf[out] = '\0';
        }
        status = SNOOPER_OK;
    }

    CFRelease(prop);
    return status;
}

static void mask_identifier(char *value) {
    if (!value) return;
    size_t len = strlen(value);
    if (len <= 4) return;
    for (size_t i = 0; i + 4 < len; ++i) {
        value[i] = '*';
    }
}

SnooperStatus snooper_system_info_read(SnooperSystemInfo *info, int reveal_identifiers) {
    if (!info) {
        return SNOOPER_ERR_INVALID;
    }

    memset(info, 0, sizeof(*info));

    if (read_sysctl_string("machdep.cpu.brand_string", info->cpu_model, sizeof(info->cpu_model)) != SNOOPER_OK) {
        return SNOOPER_ERR_UNAVAILABLE;
    }

    int arm64 = 0;
    if (read_sysctl_int("hw.optional.arm64", &arm64) == SNOOPER_OK && arm64 == 1) {
        safe_strcpy(info->cpu_architecture, sizeof(info->cpu_architecture), "ARM64");
    } else {
        read_sysctl_string("hw.machine", info->cpu_architecture, sizeof(info->cpu_architecture));
    }

    if (read_sysctl_int("hw.physicalcpu", &info->physical_cores) != SNOOPER_OK) return SNOOPER_ERR_UNAVAILABLE;
    if (read_sysctl_int("hw.logicalcpu", &info->logical_cores) != SNOOPER_OK) return SNOOPER_ERR_UNAVAILABLE;

    if (get_cf_property("IODeviceTree:/product", "product-name", info->product_name, sizeof(info->product_name)) != SNOOPER_OK) {
        safe_strcpy(info->product_name, sizeof(info->product_name), "<unavailable>");
    }

    size_t sz = sizeof(info->board_id);
    if (sysctlbyname("hw.model", info->board_id, &sz, NULL, 0) != 0) {
        if (get_cf_property("IODeviceTree:/efi/platform", "board-id", info->board_id, sizeof(info->board_id)) != SNOOPER_OK) {
            safe_strcpy(info->board_id, sizeof(info->board_id), "<unavailable>");
        }
    } else {
        info->board_id[sizeof(info->board_id) - 1] = '\0';
    }

    if (get_cf_property("IOService:/", "IOPlatformSerialNumber", info->serial_number, sizeof(info->serial_number)) != SNOOPER_OK) {
        safe_strcpy(info->serial_number, sizeof(info->serial_number), "<unavailable>");
    }

    if (get_cf_property("IOService:/", "IOPlatformUUID", info->hardware_uuid, sizeof(info->hardware_uuid)) != SNOOPER_OK) {
        safe_strcpy(info->hardware_uuid, sizeof(info->hardware_uuid), "<unavailable>");
    }

    if (!reveal_identifiers) {
        mask_identifier(info->serial_number);
        mask_identifier(info->hardware_uuid);
    }

    return SNOOPER_OK;
}
