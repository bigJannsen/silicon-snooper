#import <Cocoa/Cocoa.h>
#import "gui_bridge.h"
#import "gui_ringbuffer.h"

@interface GuiGraphView : NSView
- (instancetype)initWithFrame:(NSRect)frame
                      buffer:(GuiRingBuffer *)buffer
                        probe:(GuiCpuProbe *)probe
               intervalMillis:(NSUInteger)interval;
@end

