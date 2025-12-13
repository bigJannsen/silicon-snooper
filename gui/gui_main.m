#import <Cocoa/Cocoa.h>
#import <stddef.h>
#import <stdio.h>
#import <stdlib.h>
#import <string.h>
#import "gui_view.h"
#import "gui_ringbuffer.h"
#import "gui_telemetry_bridge.h"

static void print_gui_usage(const char *progname) {
    printf("Usage:\n");
    printf("  %s [--watch <milliseconds>] [--show-identifiers]\n", progname);
    printf("\nOptions:\n");
    printf("  --watch <ms>         Sampling interval in milliseconds (default 500).\n");
    printf("  --show-identifiers   Reveal full serial/UUID identifiers.\n");
    printf("  -h, --help           Show this help message.\n");
}

static int parse_arguments(int argc, const char **argv, int *interval_ms, int *show_identifiers) {
    *interval_ms = 500;
    *show_identifiers = 0;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--watch") == 0) {
            if (i + 1 >= argc) {
                fprintf(stderr, "Missing value for --watch.\n");
                return -1;
            }
            *interval_ms = atoi(argv[++i]);
            if (*interval_ms <= 0) {
                fprintf(stderr, "Interval must be a positive integer.\n");
                return -1;
            }
        } else if (strcmp(argv[i], "--show-identifiers") == 0) {
            *show_identifiers = 1;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_gui_usage(argv[0]);
            exit(0);
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return -1;
        }
    }

    return 0;
}

@interface GuiAppDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, assign) GuiRingBuffer *cpuBuffer;
@property (nonatomic, assign) GuiRingBuffer *gpuBuffer;
@property (nonatomic, assign) GuiCpuProbe *cpuProbe;
@property (nonatomic, assign) NSUInteger intervalMs;
@property (nonatomic, assign) GuiSystemInfo systemInfo;
@property (nonatomic, assign) BOOL showIdentifiers;
@property (nonatomic, strong) NSWindow *window;
@end

@implementation GuiAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    (void)notification;
    NSRect frame = NSMakeRect(0, 0, 900, 520);
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:(NSWindowStyleMaskTitled |
                                                         NSWindowStyleMaskClosable |
                                                         NSWindowStyleMaskMiniaturizable |
                                                         NSWindowStyleMaskResizable)
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    [self.window setTitle:@"Silicon Snooper"];

    GuiDashboardView *view = [[GuiDashboardView alloc] initWithFrame:NSMakeRect(0, 0, frame.size.width, frame.size.height)
                                                               cpuBuf:self.cpuBuffer
                                                               gpuBuf:self.gpuBuffer
                                                                 cpu:self.cpuProbe
                                                      intervalMillis:self.intervalMs
                                                         systemInfo:self.systemInfo
                                                    showIdentifiers:self.showIdentifiers];
    [self.window setContentView:view];
    [self.window center];
    [self.window makeKeyAndOrderFront:nil];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    (void)sender;
    return YES;
}

@end

int main(int argc, const char * argv[]) {
    @autoreleasepool {
        int interval_ms = 0;
        int show_identifiers = 0;
        if (parse_arguments(argc, argv, &interval_ms, &show_identifiers) != 0) {
            print_gui_usage(argv[0]);
            return 1;
        }

        size_t capacity = 60000 / (interval_ms > 0 ? (size_t)interval_ms : 1);
        if (capacity < 2) {
            capacity = 2;
        }

        GuiRingBuffer cpuBuffer;
        GuiRingBuffer gpuBuffer;
        if (gui_ring_buffer_init(&cpuBuffer, capacity) != 0 || gui_ring_buffer_init(&gpuBuffer, capacity) != 0) {
            fprintf(stderr, "Failed to allocate ring buffers.\n");
            return 1;
        }

        GuiCpuProbe cpuProbe;
        if (gui_cpu_probe_init(&cpuProbe) != 0) {
            fprintf(stderr, "Failed to initialize CPU probe.\n");
            gui_ring_buffer_destroy(&cpuBuffer);
            gui_ring_buffer_destroy(&gpuBuffer);
            return 1;
        }

        GuiSystemInfo sysInfo;
        if (gui_get_system_info(&sysInfo, show_identifiers) != 0) {
            fprintf(stderr, "Failed to read system info.\n");
            gui_cpu_probe_destroy(&cpuProbe);
            gui_ring_buffer_destroy(&cpuBuffer);
            gui_ring_buffer_destroy(&gpuBuffer);
            return 1;
        }

        NSApplication *app = [NSApplication sharedApplication];
        GuiAppDelegate *delegate = [[GuiAppDelegate alloc] init];
        delegate.cpuBuffer = &cpuBuffer;
        delegate.gpuBuffer = &gpuBuffer;
        delegate.cpuProbe = &cpuProbe;
        delegate.intervalMs = (NSUInteger)interval_ms;
        delegate.systemInfo = sysInfo;
        delegate.showIdentifiers = show_identifiers ? YES : NO;
        [app setDelegate:delegate];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        [app run];

        gui_cpu_probe_destroy(&cpuProbe);
        gui_ring_buffer_destroy(&cpuBuffer);
        gui_ring_buffer_destroy(&gpuBuffer);
    }
    return 0;
}
