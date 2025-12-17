#import "gui_view.h"
#import "gui_bridge.h"

@interface GuiGraphView ()
@property (nonatomic, assign) GuiRingBuffer *buffer;
@property (nonatomic, copy) NSString *title;
@end

@implementation GuiGraphView

- (instancetype)initWithFrame:(NSRect)frame
                         title:(NSString *)title
                        buffer:(GuiRingBuffer *)buffer {
    self = [super initWithFrame:frame];
    if (self) {
        _buffer = buffer;
        _title = [title copy];
        _metricAvailable = YES;
        _latestValue = 0.0;
    }
    return self;
}

- (BOOL)isFlipped {
    return YES;
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    NSColor *background = [NSColor colorWithCalibratedWhite:0.08 alpha:1.0];
    [background setFill];
    NSRectFill(self.bounds);

    CGFloat padding = 16.0;
    NSRect graphRect = NSInsetRect(self.bounds, padding, padding + 10.0);

    [[NSColor colorWithCalibratedWhite:0.16 alpha:1.0] setFill];
    NSBezierPath *panel = [NSBezierPath bezierPathWithRoundedRect:graphRect xRadius:8 yRadius:8];
    [panel fill];

    NSDictionary *labelAttrs = @{ NSForegroundColorAttributeName: [NSColor whiteColor],
                                  NSFontAttributeName: [NSFont monospacedDigitSystemFontOfSize:12 weight:NSFontWeightMedium] };
    NSString *titleText = self.title ?: @"Usage";
    [titleText drawAtPoint:NSMakePoint(graphRect.origin.x, graphRect.origin.y - 14)
             withAttributes:labelAttrs];

    NSString *latestText = self.metricAvailable ?
        [NSString stringWithFormat:@"Latest: %.1f%%", self.latestValue] :
        @"Latest: N/A";
    [latestText drawAtPoint:NSMakePoint(graphRect.origin.x + graphRect.size.width - 130,
                                        graphRect.origin.y - 14)
             withAttributes:labelAttrs];

    if (!_buffer || _buffer->count == 0 || !_buffer->values) {
        if (!self.metricAvailable) {
            NSDictionary *unavailAttrs = @{ NSForegroundColorAttributeName: [NSColor colorWithCalibratedRed:0.9 green:0.6 blue:0.4 alpha:1.0],
                                             NSFontAttributeName: [NSFont systemFontOfSize:13 weight:NSFontWeightSemibold] };
            NSString *msg = @"Unavailable";
            NSSize size = [msg sizeWithAttributes:unavailAttrs];
            CGFloat x = graphRect.origin.x + (graphRect.size.width - size.width) / 2.0;
            CGFloat y = graphRect.origin.y + (graphRect.size.height - size.height) / 2.0;
            [msg drawAtPoint:NSMakePoint(x, y) withAttributes:unavailAttrs];
        }
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

    NSColor *stroke = self.metricAvailable ? [NSColor colorWithCalibratedRed:0.2 green:0.7 blue:1.0 alpha:1.0]
                                           : [NSColor colorWithCalibratedWhite:0.5 alpha:0.6];
    [stroke setStroke];
    [path setLineWidth:2.0];
    [path stroke];

    if (!self.metricAvailable) {
        NSDictionary *unavailAttrs = @{ NSForegroundColorAttributeName: [NSColor colorWithCalibratedRed:0.9 green:0.6 blue:0.4 alpha:1.0],
                                         NSFontAttributeName: [NSFont systemFontOfSize:13 weight:NSFontWeightSemibold] };
        NSString *msg = @"Unavailable";
        NSSize size = [msg sizeWithAttributes:unavailAttrs];
        CGFloat x = graphRect.origin.x + (graphRect.size.width - size.width) / 2.0;
        CGFloat y = graphRect.origin.y + (graphRect.size.height - size.height) / 2.0;
        [msg drawAtPoint:NSMakePoint(x, y) withAttributes:unavailAttrs];
    }

    free(values);
}

@end

@interface GuiDashboardView ()
@property (nonatomic, assign) GuiRingBuffer *cpuBuffer;
@property (nonatomic, assign) GuiRingBuffer *gpuBuffer;
@property (nonatomic, assign) GuiTelemetry *telemetry;
@property (nonatomic, strong) GuiGraphView *cpuView;
@property (nonatomic, strong) GuiGraphView *gpuView;
@property (nonatomic, strong) NSTimer *timer;
@property (nonatomic, assign) NSUInteger intervalMillis;
@property (nonatomic, assign) SnooperSystemInfo systemInfo;
@property (nonatomic, assign) BOOL showIdentifiers;
@end

@implementation GuiDashboardView

- (instancetype)initWithFrame:(NSRect)frame
                       cpuBuf:(GuiRingBuffer *)cpuBuf
                       gpuBuf:(GuiRingBuffer *)gpuBuf
                    telemetry:(GuiTelemetry *)telemetry
              intervalMillis:(NSUInteger)interval
                 systemInfo:(SnooperSystemInfo)systemInfo
            showIdentifiers:(BOOL)showIdentifiers {
    self = [super initWithFrame:frame];
    if (self) {
        _cpuBuffer = cpuBuf;
        _gpuBuffer = gpuBuf;
        _telemetry = telemetry;
        _intervalMillis = interval;
        _systemInfo = systemInfo;
        _showIdentifiers = showIdentifiers;

        _cpuView = [[GuiGraphView alloc] initWithFrame:NSZeroRect title:@"CPU Usage (%)" buffer:_cpuBuffer];
        _gpuView = [[GuiGraphView alloc] initWithFrame:NSZeroRect title:@"GPU Usage (%)" buffer:_gpuBuffer];
        [self addSubview:_cpuView];
        [self addSubview:_gpuView];

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

- (void)layout {
    [super layout];
    CGFloat padding = 16.0;
    CGFloat infoHeight = 140.0;
    CGFloat graphAreaY = padding + infoHeight;
    CGFloat availableHeight = self.bounds.size.height - graphAreaY - padding;
    CGFloat graphWidth = (self.bounds.size.width - padding * 3) / 2.0;

    self.cpuView.frame = NSMakeRect(padding,
                                    graphAreaY,
                                    graphWidth,
                                    availableHeight);
    self.gpuView.frame = NSMakeRect(padding * 2 + graphWidth,
                                    graphAreaY,
                                    graphWidth,
                                    availableHeight);
}

- (void)startTimer {
    NSTimeInterval seconds = (double)_intervalMillis / 1000.0;
    __weak typeof(self) weakSelf = self;
    _timer = [NSTimer scheduledTimerWithTimeInterval:seconds
                                              repeats:YES
                                                block:^(NSTimer * _Nonnull timer) {
        [weakSelf pollTelemetry];
    }];
}

- (void)pollTelemetry {
    if (_telemetry) {
        SnooperSnapshot snapshot = {0};
        int rc = gui_poll_snapshot(_telemetry, &snapshot);
        if (rc == 0) {
            double cpu_used = snapshot.cpu_used_percent;
            if (cpu_used < 0.0) cpu_used = 0.0;
            if (cpu_used > 100.0) cpu_used = 100.0;
            gui_ring_buffer_push(_cpuBuffer, cpu_used);
            self.cpuView.latestValue = cpu_used;
            self.cpuView.metricAvailable = YES;

            if (snapshot.gpu_available) {
                double gpu_val = snapshot.gpu_used_percent;
                if (gpu_val < 0.0) gpu_val = 0.0;
                if (gpu_val > 100.0) gpu_val = 100.0;
                gui_ring_buffer_push(_gpuBuffer, gpu_val);
                self.gpuView.latestValue = gpu_val;
                self.gpuView.metricAvailable = YES;
            } else {
                self.gpuView.metricAvailable = NO;
            }
        }
    }

    [self.cpuView setNeedsDisplay:YES];
    [self.gpuView setNeedsDisplay:YES];
    [self setNeedsDisplay:YES];
}

- (void)drawRect:(NSRect)dirtyRect {
    [super drawRect:dirtyRect];

    [[NSColor colorWithCalibratedWhite:0.05 alpha:1.0] setFill];
    NSRectFill(self.bounds);

    CGFloat padding = 16.0;
    CGFloat infoHeight = 140.0;
    NSRect infoRect = NSMakeRect(padding, padding, self.bounds.size.width - padding * 2, infoHeight);

    NSBezierPath *panel = [NSBezierPath bezierPathWithRoundedRect:infoRect xRadius:8 yRadius:8];
    [[NSColor colorWithCalibratedWhite:0.12 alpha:1.0] setFill];
    [panel fill];

    NSDictionary *titleAttrs = @{ NSForegroundColorAttributeName: [NSColor whiteColor],
                                  NSFontAttributeName: [NSFont systemFontOfSize:14 weight:NSFontWeightSemibold] };
    [@"System Overview" drawAtPoint:NSMakePoint(infoRect.origin.x + 10, infoRect.origin.y + 10)
                      withAttributes:titleAttrs];

    NSDictionary *labelAttrs = @{ NSForegroundColorAttributeName: [NSColor colorWithCalibratedWhite:0.85 alpha:1.0],
                                  NSFontAttributeName: [NSFont monospacedDigitSystemFontOfSize:12 weight:NSFontWeightRegular] };

    CGFloat lineHeight = 18.0;
    CGFloat startY = infoRect.origin.y + 34.0;
    CGFloat col1X = infoRect.origin.x + 14.0;
    CGFloat col2X = infoRect.origin.x + infoRect.size.width / 2.0 + 8.0;

    NSArray<NSString *> *leftLines = @[
        [NSString stringWithFormat:@"CPU Model        : %s", _systemInfo.cpu_model],
        [NSString stringWithFormat:@"Architecture     : %s", _systemInfo.cpu_architecture],
        [NSString stringWithFormat:@"Cores (phys/log) : %d / %d", _systemInfo.physical_cores, _systemInfo.logical_cores],
        [NSString stringWithFormat:@"Board ID         : %s", _systemInfo.board_id]
    ];

    NSArray<NSString *> *rightLines = @[
        [NSString stringWithFormat:@"Product Name     : %s", _systemInfo.product_name],
        [NSString stringWithFormat:@"Serial Number    : %s", _systemInfo.serial_number],
        [NSString stringWithFormat:@"Hardware UUID    : %s", _systemInfo.hardware_uuid],
        _showIdentifiers ? @"Identifiers shown" : @"Identifiers masked"
    ];

    for (NSUInteger i = 0; i < [leftLines count]; ++i) {
        CGFloat y = startY + lineHeight * i;
        [leftLines[i] drawAtPoint:NSMakePoint(col1X, y) withAttributes:labelAttrs];
    }

    for (NSUInteger i = 0; i < [rightLines count]; ++i) {
        CGFloat y = startY + lineHeight * i;
        [rightLines[i] drawAtPoint:NSMakePoint(col2X, y) withAttributes:labelAttrs];
    }
}

@end
