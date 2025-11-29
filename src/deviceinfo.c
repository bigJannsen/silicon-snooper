#include <IOKit/IOKitLib.h>
#include <CoreFoundation/CoreFoundation.h>
#include <sys/sysctl.h>
#include <string.h>
#include <stdio.h>


//  INTERN: CF-Property-Reader

static int get_cf_property(const char *path, const char *key, char *buf, size_t size)
{
    if (!buf || size == 0) return -1;

    io_registry_entry_t entry = IORegistryEntryFromPath(kIOMainPortDefault, path);
    if (!entry) return -1;

    CFStringRef cfkey = CFStringCreateWithCString(NULL, key, kCFStringEncodingUTF8);
    CFTypeRef prop = IORegistryEntryCreateCFProperty(entry, cfkey, kCFAllocatorDefault, 0);

    CFRelease(cfkey);
    IOObjectRelease(entry);

    if (!prop) return -1;

    // Read CFString
    if (CFGetTypeID(prop) == CFStringGetTypeID()) {
        CFStringGetCString((CFStringRef)prop, buf, size, kCFStringEncodingUTF8);
        CFRelease(prop);
        return 0;
    }

    // 2) CFData → could be binary or text data
    if (CFGetTypeID(prop) == CFDataGetTypeID()) {
        CFDataRef data = (CFDataRef)prop;
        CFIndex len = CFDataGetLength(data);
        const UInt8 *bytes = CFDataGetBytePtr(data);

        // ASCII proofing
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
            // HEX-Dump
            size_t out = 0;
            for (CFIndex i = 0; i < len && out + 3 < size; i++)
                out += snprintf(buf + out, size - out, "%02X", bytes[i]);
            buf[out] = '\0';
        }

        CFRelease(prop);
        return 0;
    }

    CFRelease(prop);
    return -1;
}

//  PUBLIC: Serial number

int get_serial(char *buf, size_t size)
{
    return get_cf_property("IOService:/", "IOPlatformSerialNumber", buf, size);
}

//  PUBLIC: Hardware UUID

int get_hardware_uuid(char *buf, size_t size)
{
    return get_cf_property("IOService:/", "IOPlatformUUID", buf, size);
}

//  Product Name

int get_product_name(char *buf, size_t size)
{
    return get_cf_property("IODeviceTree:/product", "product-name", buf, size);
}

//  PUBLIC: Board-ID (→ Apple Silicon: only hw.model available)

int get_board_id(char *buf, size_t size)
{
    // real board-id is not available, hw-model is the official
    size_t sz = size;
    if (sysctlbyname("hw.model", buf, &sz, NULL, 0) == 0) {
        buf[size - 1] = '\0';
        return 0;
    }

    // Fallback for Intel Kernels
    if (get_cf_property("IODeviceTree:/efi/platform", "board-id", buf, size) == 0)
        return 0;

    return -1;
}
