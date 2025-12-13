#import <Cocoa/Cocoa.h>
#import "gui_ringbuffer.h"
#import "gui_telemetry_bridge.h"

@interface GuiGraphView : NSView
- (instancetype)initWithFrame:(NSRect)frame
                         title:(NSString *)title
                        buffer:(GuiRingBuffer *)buffer;

@property (nonatomic, assign) double latestValue;
@property (nonatomic, assign) BOOL metricAvailable;
@end

@interface GuiDashboardView : NSView
- (instancetype)initWithFrame:(NSRect)frame
                       cpuBuf:(GuiRingBuffer *)cpuBuf
                       gpuBuf:(GuiRingBuffer *)gpuBuf
                         cpu:(GuiCpuProbe *)cpuProbe
                         intervalMillis:(NSUInteger)interval
                    systemInfo:(GuiSystemInfo)systemInfo
             showIdentifiers:(BOOL)showIdentifiers;
@end

