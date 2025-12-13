#import <Cocoa/Cocoa.h>
#import <stddef.h>
#import <stdio.h>
#import <stdlib.h>
#import <string.h>
#import "gui_view.h"
#import "gui_ringbuffer.h"
#import "gui_bridge.h"

static void print_gui_usage(const char *progname) {
    printf("Usage:\n");
    printf("  %s [--watch <milliseconds>]\n", progname);
    printf("\nOptions:\n");
    printf("  --watch <ms>   Sampling interval in milliseconds (default 500).\n");
    printf("  -h, --help     Show this help message.\n");
}

static int parse_interval(int argc, const char **argv, int *interval_ms) {
    *interval_ms = 500;
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
@property (nonatomic, assign) GuiRingBuffer *buffer;
@property (nonatomic, assign) GuiCpuProbe *probe;
@property (nonatomic, assign) NSUInteger intervalMs;
@property (nonatomic, strong) NSWindow *window;
@end

@implementation GuiAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    (void)notification;
    NSRect frame = NSMakeRect(0, 0, 520, 220);
    self.window = [[NSWindow alloc] initWithContentRect:frame
                                              styleMask:(NSWindowStyleMaskTitled |
                                                         NSWindowStyleMaskClosable |
                                                         NSWindowStyleMaskMiniaturizable)
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    [self.window setTitle:@"Silicon Snooper - CPU Graph"];

    GuiGraphView *view = [[GuiGraphView alloc] initWithFrame:NSMakeRect(0, 0, 520, 220)
                                                      buffer:self.buffer
                                                        probe:self.probe
                                               intervalMillis:self.intervalMs];
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
        if (parse_interval(argc, argv, &interval_ms) != 0) {
            print_gui_usage(argv[0]);
            return 1;
        }

        size_t capacity = 60000 / (interval_ms > 0 ? (size_t)interval_ms : 1);
        if (capacity < 2) {
            capacity = 2;
        }

        GuiRingBuffer buffer;
        if (gui_ring_buffer_init(&buffer, capacity) != 0) {
            fprintf(stderr, "Failed to allocate ring buffer.\n");
            return 1;
        }

        GuiCpuProbe probe;
        if (gui_cpu_probe_init(&probe) != 0) {
            fprintf(stderr, "Failed to initialize CPU probe.\n");
            gui_ring_buffer_destroy(&buffer);
            return 1;
        }

        NSApplication *app = [NSApplication sharedApplication];
        GuiAppDelegate *delegate = [[GuiAppDelegate alloc] init];
        delegate.buffer = &buffer;
        delegate.probe = &probe;
        delegate.intervalMs = (NSUInteger)interval_ms;
        [app setDelegate:delegate];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];
        [app run];

        gui_cpu_probe_destroy(&probe);
        gui_ring_buffer_destroy(&buffer);
    }
    return 0;
}
