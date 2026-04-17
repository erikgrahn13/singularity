#include "CocoaWindow.h"
#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#include <functional>

@interface SingularityWindowDelegate : NSObject<NSWindowDelegate>
@property (nonatomic, assign) bool* running;
@property (nonatomic, assign) std::function<void()>* onClose;
@end

@implementation SingularityWindowDelegate
- (BOOL)windowShouldClose:(NSWindow*)sender {
    if (self.onClose && *self.onClose)
        (*self.onClose)();
    else {
        *self.running = false;
        [NSApp stop:nil];
    }
    return YES;
}
@end

@interface SingularityContentView : NSView
@property (nonatomic, assign) std::function<void(int,int,unsigned int)>* onMouseDown;
@property (nonatomic, assign) std::function<void(int,int,unsigned int)>* onMouseUp;
@property (nonatomic, assign) std::function<void(int,int)>*              onMouseMove;
@property (nonatomic, assign) std::function<DrawingContent()>*           onFrame;
@end

@implementation SingularityContentView

- (BOOL)wantsUpdateLayer { return YES; }
- (BOOL)wantsLayer       { return YES; }

- (void)updateLayer {
    if (!self.onFrame || !*self.onFrame) return;
    DrawingContent dc = (*self.onFrame)();
    if (!dc.contentAddres) return;
    size_t bytesPerRow = dc.contentBytes;
    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGDataProviderRef dp = CGDataProviderCreateWithData(nullptr, dc.contentAddres, bytesPerRow * dc.height, nullptr);
    CGImageRef img = CGImageCreate(dc.width, dc.height, 8, 32, bytesPerRow,
                                   cs, kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst,
                                   dp, nullptr, false, kCGRenderingIntentDefault);
    self.layer.contents = (__bridge id)img;
    CGImageRelease(img);
    CGDataProviderRelease(dp);
    CGColorSpaceRelease(cs);
}

- (NSPoint)localPoint:(NSEvent*)e {
    NSPoint p = [self convertPoint:e.locationInWindow fromView:nil];
    return NSMakePoint(p.x, self.bounds.size.height - p.y);
}
- (void)mouseDown:(NSEvent*)e {
    NSPoint p = [self localPoint:e];
    if (self.onMouseDown && *self.onMouseDown) (*self.onMouseDown)((int)p.x, (int)p.y, 1);
}
- (void)mouseUp:(NSEvent*)e {
    NSPoint p = [self localPoint:e];
    if (self.onMouseUp && *self.onMouseUp) (*self.onMouseUp)((int)p.x, (int)p.y, 1);
}
- (void)mouseMoved:(NSEvent*)e    { NSPoint p = [self localPoint:e]; if (self.onMouseMove && *self.onMouseMove) (*self.onMouseMove)((int)p.x, (int)p.y); }
- (void)mouseDragged:(NSEvent*)e  { [self mouseMoved:e]; }

@end

@interface SingularityDisplayLinkTarget : NSObject
@property (nonatomic, assign) SingularityContentView* view;
@end

@implementation SingularityDisplayLinkTarget
- (void)displayLinkFired:(CADisplayLink*)link { [self.view setNeedsDisplay:YES]; }
@end

struct CocoaWindowImpl {
    NSWindow*                     window            = nil;
    SingularityWindowDelegate*    delegate          = nil;
    SingularityContentView*       view              = nil;
    CADisplayLink*                displayLink       = nil;
    SingularityDisplayLinkTarget* displayLinkTarget = nil;
    bool                          running           = false;
    bool                          embedded          = false;
    int                           width             = 0;
    int                           height            = 0;
    std::function<void(int,int,unsigned int)> onMouseDown;
    std::function<void(int,int,unsigned int)> onMouseUp;
    std::function<void(int,int)>              onMouseMove;
    std::function<DrawingContent()>           onFrame;
    std::function<void()>                     onClose;
};

CocoaWindow::CocoaWindow(const std::string& title, int width, int height,
                         void* parentNSView, void* ownerNSWindow)
    : m_impl(std::make_unique<CocoaWindowImpl>())
{
    m_impl->width    = width;
    m_impl->height   = height;
    m_impl->embedded = (parentNSView != nullptr);

    NSRect frame = NSMakeRect(0, 0, width, height);

    m_impl->view = [[SingularityContentView alloc] initWithFrame:frame];
    m_impl->view.onMouseDown = &m_impl->onMouseDown;
    m_impl->view.onMouseUp   = &m_impl->onMouseUp;
    m_impl->view.onMouseMove = &m_impl->onMouseMove;
    m_impl->view.onFrame     = &m_impl->onFrame;

    if (m_impl->embedded) {
        [m_impl->view setWantsLayer:YES];
        [(__bridge NSView*)parentNSView addSubview:m_impl->view];
    } else {
        [NSApplication sharedApplication];
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

        m_impl->window = [[NSWindow alloc]
            initWithContentRect:frame
            styleMask:(NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
                       NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable)
            backing:NSBackingStoreBuffered defer:NO];

        m_impl->delegate = [[SingularityWindowDelegate alloc] init];
        m_impl->delegate.running = &m_impl->running;
        m_impl->delegate.onClose = &m_impl->onClose;
        m_impl->window.delegate  = m_impl->delegate;

        [m_impl->window setContentView:m_impl->view];
        [m_impl->window setAcceptsMouseMovedEvents:YES];
        [m_impl->window setTitle:[NSString stringWithUTF8String:title.c_str()]];
        [m_impl->window center];

        if (ownerNSWindow) {
            NSWindow* owner = (__bridge NSWindow*)ownerNSWindow;
            [owner addChildWindow:m_impl->window ordered:NSWindowAbove];
            m_impl->window.level = NSFloatingWindowLevel;
        }

        [m_impl->window makeKeyAndOrderFront:nil];
        [NSApp activateIgnoringOtherApps:YES];
    }
}

CocoaWindow::~CocoaWindow() {
    if (m_impl->displayLink) [m_impl->displayLink invalidate];
    if (m_impl->embedded) [m_impl->view removeFromSuperview];
    else if (m_impl->window) [m_impl->window close];
}

void CocoaWindow::run() {
    if (m_impl->embedded) return;
    m_impl->running = true;
    if (!m_impl->displayLink && m_impl->onFrame) {
        m_impl->displayLinkTarget = [[SingularityDisplayLinkTarget alloc] init];
        m_impl->displayLinkTarget.view = m_impl->view;
        m_impl->displayLink = [m_impl->view displayLinkWithTarget:m_impl->displayLinkTarget
                                                         selector:@selector(displayLinkFired:)];
        [m_impl->displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
    }
    [NSApp run];
}

void CocoaWindow::close() { m_impl->running = false; [NSApp stop:nil]; }

void CocoaWindow::setOnFrame(std::function<DrawingContent()> cb) {
    m_impl->onFrame = std::move(cb);
    if (m_impl->embedded && !m_impl->displayLink && m_impl->onFrame) {
        m_impl->displayLinkTarget = [[SingularityDisplayLinkTarget alloc] init];
        m_impl->displayLinkTarget.view = m_impl->view;
        m_impl->displayLink = [NSScreen.mainScreen displayLinkWithTarget:m_impl->displayLinkTarget
                                                                selector:@selector(displayLinkFired:)];
        [m_impl->displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
    }
}

void CocoaWindow::setOnMouseDown(std::function<void(int,int,unsigned int)> cb) { m_impl->onMouseDown = std::move(cb); }
void CocoaWindow::setOnMouseUp  (std::function<void(int,int,unsigned int)> cb) { m_impl->onMouseUp   = std::move(cb); }
void CocoaWindow::setOnMouseMove(std::function<void(int,int)> cb)              { m_impl->onMouseMove = std::move(cb); }
void CocoaWindow::setOnClose    (std::function<void()> cb)                     { m_impl->onClose     = std::move(cb); }

int   CocoaWindow::width()        const { return m_impl->width; }
int   CocoaWindow::height()       const { return m_impl->height; }
void* CocoaWindow::nativeHandle() const { return (__bridge void*)m_impl->window; }
