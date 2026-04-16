#include "CocoaWindow.h"
#import <AppKit/AppKit.h>
#include <functional>

// ---------------------------------------------------------------------------
// Delegate — handles window close
// ---------------------------------------------------------------------------
@interface SingularityWindowDelegate : NSObject<NSWindowDelegate>
@property (nonatomic, assign) bool* running;
@end

@implementation SingularityWindowDelegate
- (BOOL)windowShouldClose:(NSWindow*)sender {
    *self.running = false;
    [NSApp stop:nil];
    return YES;
}
@end

// ---------------------------------------------------------------------------
// Custom view — captures mouse events
// ---------------------------------------------------------------------------
@interface SingularityContentView : NSView
@property (nonatomic, assign) std::function<void(int,int,unsigned int)>* onMouseDown;
@property (nonatomic, assign) std::function<void(int,int,unsigned int)>* onMouseUp;
@end

@implementation SingularityContentView

- (BOOL)acceptsFirstResponder { return YES; }

- (void)dispatchDown:(NSEvent*)e button:(unsigned int)btn {
    NSPoint p = [self convertPoint:e.locationInWindow fromView:nil];
    if (self.onMouseDown && *self.onMouseDown)
        (*self.onMouseDown)((int)p.x, (int)(self.bounds.size.height - p.y), btn);
}
- (void)dispatchUp:(NSEvent*)e button:(unsigned int)btn {
    NSPoint p = [self convertPoint:e.locationInWindow fromView:nil];
    if (self.onMouseUp && *self.onMouseUp)
        (*self.onMouseUp)((int)p.x, (int)(self.bounds.size.height - p.y), btn);
}
- (void)mouseDown:(NSEvent*)e      { [self dispatchDown:e button:1]; }
- (void)mouseUp:(NSEvent*)e        { [self dispatchUp:e   button:1]; }
- (void)rightMouseDown:(NSEvent*)e { [self dispatchDown:e button:3]; }
- (void)rightMouseUp:(NSEvent*)e   { [self dispatchUp:e   button:3]; }
- (void)otherMouseDown:(NSEvent*)e { [self dispatchDown:e button:2]; }
- (void)otherMouseUp:(NSEvent*)e   { [self dispatchUp:e   button:2]; }

@end

// ---------------------------------------------------------------------------
// PIMPL struct
// ---------------------------------------------------------------------------
struct CocoaWindowImpl {
    NSWindow*                    window   = nil;
    SingularityWindowDelegate*   delegate = nil;
    SingularityContentView*      view     = nil;
    bool                         running  = false;
    int                          width    = 0;
    int                          height   = 0;
    std::function<void(int,int,unsigned int)> onMouseDown;
    std::function<void(int,int,unsigned int)> onMouseUp;
};

// ---------------------------------------------------------------------------
// CocoaWindow implementation
// ---------------------------------------------------------------------------
CocoaWindow::CocoaWindow(const std::string& title, int width, int height)
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
    m_impl->window.delegate  = m_impl->delegate;

    m_impl->view = [[SingularityContentView alloc] initWithFrame:frame];
    m_impl->view.onMouseDown = &m_impl->onMouseDown;
    m_impl->view.onMouseUp   = &m_impl->onMouseUp;
    [m_impl->window setContentView:m_impl->view];

    [m_impl->window setTitle:[NSString stringWithUTF8String:title.c_str()]];
    [m_impl->window center];
    [m_impl->window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}

CocoaWindow::~CocoaWindow() {
    if (m_impl->window)
        [m_impl->window close];
}

void CocoaWindow::run() {
    m_impl->running = true;
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
