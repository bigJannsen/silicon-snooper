#include <stdio.h>
#include <stdlib.h>
#include <sys/sysctl.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>

void print_macOS_version() {
    char version[256];
    size_t size = sizeof(version);
    if (sysctlbyname("kern.osproductversion", version, &size, NULL, 0) == 0) {
        printf("macOS Version: %s\n", version);
    } else {
        printf("macOS Version: unknown\n");
    }
}

void print_cpu_core_count() {
    int cores = 0;
    size_t size = sizeof(cores);
    if (sysctlbyname("hw.ncpu", &cores, &size, NULL, 0) == 0) {
        printf("CPU Cores (Total): %d\n", cores);
    } else {
        perror("sysctl hw.ncpu");
    }
}

void print_battery_status() {
    CFTypeRef info = IOPSCopyPowerSourcesInfo();
    if (!info) {
        printf("Unable to access power sources info.\n");
        return;
    }

    CFArrayRef list = IOPSCopyPowerSourcesList(info);
    if (!list || CFArrayGetCount(list) == 0) {
        printf("No battery detected.\n");
        return;
    }

    CFDictionaryRef battery = IOPSGetPowerSourceDescription(info, CFArrayGetValueAtIndex(list, 0));
    if (!battery) {
        printf("Battery info not available.\n");
        return;
    }

    int current = 0, max = 1;
    CFNumberRef cur = CFDictionaryGetValue(battery, CFSTR(kIOPSCurrentCapacityKey));
    CFNumberRef maxc = CFDictionaryGetValue(battery, CFSTR(kIOPSMaxCapacityKey));

    if (cur && maxc) {
        CFNumberGetValue(cur, kCFNumberIntType, &current);
        CFNumberGetValue(maxc, kCFNumberIntType, &max);
        printf("Battery: %d%%\n", (current * 100) / max);
    } else {
        printf("Battery values missing.\n");
    }
}

int main() {
    printf("=== silicon-snooper ===\n\n");

    print_macOS_version();
    print_cpu_core_count();
    print_battery_status();

    return 0;
}
