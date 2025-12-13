#import "gui_view.h"

@interface GuiGraphView ()
@property (nonatomic, assign) GuiRingBuffer *buffer;
@property (nonatomic, assign) GuiCpuProbe *probe;
@property (nonatomic, strong) NSTimer *timer;
@property (nonatomic, assign) NSUInteger intervalMillis;
@end

@implementation GuiGraphView

- (instancetype)initWithFrame:(NSRect)frame
                      buffer:(GuiRingBuffer *)buffer
                        probe:(GuiCpuProbe *)probe
               intervalMillis:(NSUInteger)interval {
    self = [super initWithFrame:frame];
    if (self) {
        _buffer = buffer;
        _probe = probe;
        _intervalMillis = interval;
        [self startTimer];
    }
    return self;
}

- (BOOL)isFlipped {
    return YES;
}

- (void)dealloc {
    [_timer invalidate];
}

- (void)startTimer {
    NSTimeInterval seconds = (double)_intervalMillis / 1000.0;
    __weak typeof(self) weakSelf = self;
    _timer = [NSTimer scheduledTimerWithTimeInterval:seconds
                                              repeats:YES
                                                block:^(NSTimer * _Nonnull timer) {
        [weakSelf pollCpu];
    }];
}

- (void)pollCpu {
    if (!_probe || !_buffer) {
        return;
    }

    CpuUsage overall = {0};
    int rc = gui_cpu_probe_sample(_probe, &overall);
    if (rc != 0) {
        // First sample returns 1 (no delta yet) or an error occurred.
        return;
    }

    double used_percent = 100.0 - overall.idle;
    if (overall.user == 0.0 && overall.system == 0.0 && overall.idle == 0.0) {
        used_percent = 0.0;
    }

    if (used_percent < 0.0) {
        used_percent = 0.0;
    }
    if (used_percent > 100.0) {
        used_percent = 100.0;
    }

    gui_ring_buffer_push(_buffer, used_percent);
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    NSColor *background = [NSColor colorWithCalibratedWhite:0.08 alpha:1.0];
    [background setFill];
    NSRectFill(self.bounds);

    CGFloat padding = 16.0;
    NSRect graphRect = NSInsetRect(self.bounds, padding, padding);

    [[NSColor colorWithCalibratedWhite:0.2 alpha:1.0] setFill];
    NSRectFill(graphRect);

    NSDictionary *labelAttrs = @{ NSForegroundColorAttributeName: [NSColor whiteColor],
                                  NSFontAttributeName: [NSFont monospacedDigitSystemFontOfSize:12 weight:NSFontWeightMedium] };
    NSString *title = @"CPU Usage (%)";
    [title drawAtPoint:NSMakePoint(graphRect.origin.x, graphRect.origin.y - 14)
        withAttributes:labelAttrs];

    if (!_buffer || _buffer->count == 0) {
        return;
    }

    double *values = (double *)calloc(_buffer->count, sizeof(double));
    if (!values) {
        return;
    }

    size_t copied = gui_ring_buffer_copy(_buffer, values, _buffer->count);
    if (copied == 0) {
        free(values);
        return;
    }

    NSBezierPath *path = [NSBezierPath bezierPath];
    CGFloat stepX = graphRect.size.width / (CGFloat)(copied > 1 ? (copied - 1) : 1);

    for (size_t i = 0; i < copied; ++i) {
        double value = values[i];
        if (value < 0.0) value = 0.0;
        if (value > 100.0) value = 100.0;
        CGFloat x = graphRect.origin.x + (CGFloat)i * stepX;
        CGFloat y = graphRect.origin.y + graphRect.size.height - (CGFloat)(value / 100.0) * graphRect.size.height;
        if (i == 0) {
            [path moveToPoint:NSMakePoint(x, y)];
        } else {
            [path lineToPoint:NSMakePoint(x, y)];
        }
    }

    [[NSColor colorWithCalibratedRed:0.2 green:0.7 blue:1.0 alpha:1.0] setStroke];
    [path setLineWidth:2.0];
    [path stroke];

    NSString *latest = [NSString stringWithFormat:@"Latest: %.1f%%", values[copied - 1]];
    [latest drawAtPoint:NSMakePoint(graphRect.origin.x + graphRect.size.width - 120,
                                    graphRect.origin.y - 14)
        withAttributes:labelAttrs];

    free(values);
}

@end
