#include "CocoaWindow.h"
#import <AppKit/AppKit.h>
#import <QuartzCore/QuartzCore.h>
#include <functional>

// ---------------------------------------------------------------------------
// Delegate — handles window close
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Custom view — captures mouse events
// ---------------------------------------------------------------------------
@interface SingularityContentView : NSView
@property (nonatomic, assign) std::function<void(int,int,unsigned int)>* onMouseDown;
@property (nonatomic, assign) std::function<void(int,int,unsigned int)>* onMouseUp;
@property (nonatomic, assign) std::function<void(int,int)>*              onMouseMove;
@property (nonatomic, assign) std::function<DrawingContent()>*           onFrame;
@end

@implementation SingularityContentView

- (BOOL)wantsUpdateLayer { return YES; }
- (BOOL)wantsLayer { return YES; }

- (void)updateLayer {
    if (!self.onFrame || !*self.onFrame) return;
    DrawingContent dc = (*self.onFrame)();
    static int frameCount = 0;
    if (frameCount++ < 5)
        NSLog(@"[CocoaWindow] updateLayer frame=%d addr=%p w=%d h=%d stride=%zu",
              frameCount, dc.contentAddres, dc.width, dc.height, (size_t)dc.contentBytes);
    if (!dc.contentAddres) return;

    size_t bytesPerRow = dc.contentBytes; // contentBytes is the row stride
    size_t totalBytes  = bytesPerRow * dc.height;

    CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
    CGDataProviderRef dp = CGDataProviderCreateWithData(nullptr, dc.contentAddres, totalBytes, nullptr);
    CGImageRef img = CGImageCreate(dc.width, dc.height, 8, 32, bytesPerRow,
                                   cs, kCGBitmapByteOrder32Little | kCGImageAlphaPremultipliedFirst,
                                   dp, nullptr, false, kCGRenderingIntentDefault);
    self.layer.contents = (__bridge id)img;
    CGImageRelease(img);
    CGDataProviderRelease(dp);
    CGColorSpaceRelease(cs);
}

- (NSPoint)flippedPoint:(NSEvent*)e {
    return NSMakePoint([self convertPoint:e.locationInWindow fromView:nil].x,
                       self.bounds.size.height - [self convertPoint:e.locationInWindow fromView:nil].y);
}
- (void)dispatchDown:(NSEvent*)e button:(unsigned int)btn {
    NSPoint p = [self flippedPoint:e];
    if (self.onMouseDown && *self.onMouseDown)
        (*self.onMouseDown)((int)p.x, (int)p.y, btn);
}
- (void)dispatchUp:(NSEvent*)e button:(unsigned int)btn {
    NSPoint p = [self flippedPoint:e];
    if (self.onMouseUp && *self.onMouseUp)
        (*self.onMouseUp)((int)p.x, (int)p.y, btn);
}
- (void)mouseMoved:(NSEvent*)e {
    NSPoint p = [self flippedPoint:e];
    if (self.onMouseMove && *self.onMouseMove)
        (*self.onMouseMove)((int)p.x, (int)p.y);
}
- (void)mouseDragged:(NSEvent*)e      { [self mouseMoved:e]; }
- (void)mouseDown:(NSEvent*)e         { [self dispatchDown:e button:1]; }
- (void)mouseUp:(NSEvent*)e           { [self dispatchUp:e   button:1]; }
- (void)rightMouseDown:(NSEvent*)e    { [self dispatchDown:e button:3]; }
- (void)rightMouseUp:(NSEvent*)e      { [self dispatchUp:e   button:3]; }
- (void)otherMouseDown:(NSEvent*)e    { [self dispatchDown:e button:2]; }
- (void)otherMouseUp:(NSEvent*)e      { [self dispatchUp:e   button:2]; }

@end

// ---------------------------------------------------------------------------
// CADisplayLink target
// ---------------------------------------------------------------------------
@interface SingularityDisplayLinkTarget : NSObject
@property (nonatomic, assign) SingularityContentView* view;
- (void)displayLinkFired:(CADisplayLink*)link;
@end

@implementation SingularityDisplayLinkTarget
- (void)displayLinkFired:(CADisplayLink*)link {
    [self.view setNeedsDisplay:YES];
}
@end

// ---------------------------------------------------------------------------
// PIMPL struct
// ---------------------------------------------------------------------------
struct CocoaWindowImpl {
    NSWindow*                     window            = nil;
    SingularityWindowDelegate*    delegate          = nil;
    SingularityContentView*       view              = nil;
    CADisplayLink*                displayLink       = nil;
    SingularityDisplayLinkTarget* displayLinkTarget = nil;
    bool                          running           = false;
    int                           width             = 0;
    int                           height            = 0;
    std::function<void(int,int,unsigned int)> onMouseDown;
    std::function<void(int,int,unsigned int)> onMouseUp;
    std::function<void(int,int)>              onMouseMove;
    std::function<DrawingContent()>           onFrame;
    std::function<void()>                     onClose;
    bool                          embedded          = false;
};

// ---------------------------------------------------------------------------
// CocoaWindow implementation
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Embedded constructor — for VST3/plugin: attaches as subview of parentNSView
// ---------------------------------------------------------------------------
CocoaWindow::CocoaWindow(int width, int height, void* parentNSView)
    : m_impl(std::make_unique<CocoaWindowImpl>())
{
    m_impl->width    = width;
    m_impl->height   = height;
    m_impl->embedded = true;

    NSView* parent = (__bridge NSView*)parentNSView;
    NSLog(@"[CocoaWindow] embedded ctor: parent=%@ frame=%@ window=%@",
          parent, NSStringFromRect(parent.frame), parent.window);

    NSRect frame = NSMakeRect(0, 0, width, height);

    m_impl->view = [[SingularityContentView alloc] initWithFrame:frame];
    m_impl->view.onMouseDown = &m_impl->onMouseDown;
    m_impl->view.onMouseUp   = &m_impl->onMouseUp;
    m_impl->view.onMouseMove = &m_impl->onMouseMove;
    m_impl->view.onFrame     = &m_impl->onFrame;
    [m_impl->view setWantsLayer:YES];
    [parent addSubview:m_impl->view];
    NSLog(@"[CocoaWindow] view added as subview, layer=%@", m_impl->view.layer);
}

// ---------------------------------------------------------------------------
// Standalone constructor — creates its own NSWindow
// ---------------------------------------------------------------------------
CocoaWindow::CocoaWindow(const std::string& title, int width, int height, IWindow* owner)
    : m_impl(std::make_unique<CocoaWindowImpl>())
{
    m_impl->width  = width;
    m_impl->height = height;

    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    NSRect frame = NSMakeRect(0, 0, width, height);

    m_impl->window = [[NSWindow alloc]
        initWithContentRect:frame
        styleMask:(NSWindowStyleMaskTitled      |
                   NSWindowStyleMaskClosable    |
                   NSWindowStyleMaskResizable   |
                   NSWindowStyleMaskMiniaturizable)
        backing:NSBackingStoreBuffered
        defer:NO];

    m_impl->delegate = [[SingularityWindowDelegate alloc] init];
    m_impl->delegate.running = &m_impl->running;
    m_impl->delegate.onClose = &m_impl->onClose;
    m_impl->window.delegate  = m_impl->delegate;

    if (CocoaWindow* ownerCW = dynamic_cast<CocoaWindow*>(owner)) {
        // Make this a child/modal sheet of the owner
        NSWindow* parentNSWin = ownerCW->m_impl->window;
        [parentNSWin addChildWindow:m_impl->window ordered:NSWindowAbove];
        m_impl->window.level = NSFloatingWindowLevel;
    }

    m_impl->view = [[SingularityContentView alloc] initWithFrame:frame];
    m_impl->view.onMouseDown  = &m_impl->onMouseDown;
    m_impl->view.onMouseUp    = &m_impl->onMouseUp;
    m_impl->view.onMouseMove  = &m_impl->onMouseMove;
    m_impl->view.onFrame      = &m_impl->onFrame;
    [m_impl->window setContentView:m_impl->view];
    [m_impl->window setAcceptsMouseMovedEvents:YES];

    [m_impl->window setTitle:[NSString stringWithUTF8String:title.c_str()]];
    [m_impl->window center];
    [m_impl->window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

void CocoaWindow::setOnFrame(std::function<DrawingContent()> cb) {
    m_impl->onFrame = std::move(cb);
    // In embedded mode the host runs the event loop — start the display link now.
    if (m_impl->embedded && !m_impl->displayLink && m_impl->onFrame) {
        NSLog(@"[CocoaWindow] starting display link (embedded)");
        m_impl->displayLinkTarget = [[SingularityDisplayLinkTarget alloc] init];
        m_impl->displayLinkTarget.view = m_impl->view;
        m_impl->displayLink = [NSScreen.mainScreen displayLinkWithTarget:m_impl->displayLinkTarget
                                                                selector:@selector(displayLinkFired:)];
        [m_impl->displayLink addToRunLoop:[NSRunLoop mainRunLoop]
                                  forMode:NSRunLoopCommonModes];
        NSLog(@"[CocoaWindow] display link=%@", m_impl->displayLink);
    }
}

void CocoaWindow::setOnMouseMove(std::function<void(int, int)> cb) {
    m_impl->onMouseMove = std::move(cb);
}

void CocoaWindow::setOnClose(std::function<void()> cb) {
    m_impl->onClose = std::move(cb);
}

CocoaWindow::~CocoaWindow() {
    if (m_impl->displayLink) {
        [m_impl->displayLink invalidate];
        m_impl->displayLink = nil;
    }
    if (m_impl->embedded) {
        [m_impl->view removeFromSuperview];
    } else if (m_impl->window) {
        [m_impl->window close];
    }
}

void CocoaWindow::run() {
    if (m_impl->embedded) return; // host owns the event loop

    m_impl->running = true;

    // Window is on screen now — safe to create the display link
    if (!m_impl->displayLink && m_impl->onFrame) {
        m_impl->displayLinkTarget = [[SingularityDisplayLinkTarget alloc] init];
        m_impl->displayLinkTarget.view = m_impl->view;
        m_impl->displayLink = [m_impl->view displayLinkWithTarget:m_impl->displayLinkTarget
                                                         selector:@selector(displayLinkFired:)];
        [m_impl->displayLink addToRunLoop:[NSRunLoop mainRunLoop]
                                  forMode:NSRunLoopCommonModes];
    }

    [NSApp run];
}

void CocoaWindow::close() {
    m_impl->running = false;
    [NSApp stop:nil];
}

int CocoaWindow::width()  const { return m_impl->width; }
int CocoaWindow::height() const { return m_impl->height; }

void CocoaWindow::setOnMouseDown(std::function<void(int,int,unsigned int)> cb) {
    m_impl->onMouseDown = std::move(cb);
}
void CocoaWindow::setOnMouseUp(std::function<void(int,int,unsigned int)> cb) {
    m_impl->onMouseUp = std::move(cb);
}

