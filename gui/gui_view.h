#import <Cocoa/Cocoa.h>
#import "gui_ringbuffer.h"
#import "gui_bridge.h"

@interface GuiGraphView : NSView
- (instancetype)initWithFrame:(NSRect)frame
                         title:(NSString *)title
                        buffer:(GuiRingBuffer *)buffer;

@property (nonatomic, assign) double latestValue;
@property (nonatomic, assign) BOOL metricAvailable;
@property (nonatomic, copy) NSString *unavailableLabel;
@end

@interface GuiDashboardView : NSView
- (instancetype)initWithFrame:(NSRect)frame
                       cpuBuf:(GuiRingBuffer *)cpuBuf
                       gpuBuf:(GuiRingBuffer *)gpuBuf
                    telemetry:(GuiTelemetry *)telemetry
              intervalMillis:(NSUInteger)interval
                 systemInfo:(SnooperSystemInfo)systemInfo
            showIdentifiers:(BOOL)showIdentifiers;
@end
