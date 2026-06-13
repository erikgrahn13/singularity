// AppKit window implementation using Objective-C++
#include "../IWindow.h"
#include "AppKitWindow.h"
#import <AppKit/AppKit.h>
#import <QuartzCore/CADisplayLink.h>
#import <QuartzCore/CAMetalLayer.h>
#include <memory>

extern "C" void* createMetalLayerForView(void* nsView) {
    NSView* view = (__bridge NSView*)nsView;
    [view setWantsLayer:YES];

    CAMetalLayer* metalLayer = [CAMetalLayer layer];
    metalLayer.frame = view.bounds;
    metalLayer.contentsScale = view.window.backingScaleFactor;
    [view setLayer:metalLayer];

    return (__bridge void*)metalLayer;
}

// Custom NSView subclass to capture mouse events
@interface EventView : NSView
@property (nonatomic, assign) std::function<void(int, int)>* onMouseDown;
@property (nonatomic, assign) std::function<void(int, int)>* onMouseUp;
@property (nonatomic, assign) std::function<void(int, int)>* onMouseMove;
@property (nonatomic, assign) std::function<void(float, float)>* onMouseWheel;
@end

@implementation EventView

- (BOOL)acceptsFirstResponder { return YES; }
- (BOOL)acceptsFirstMouse:(NSEvent*)event { return YES; }

- (NSPoint)flippedLocation:(NSEvent*)event {
    NSPoint loc = [self convertPoint:[event locationInWindow] fromView:nil];
    // Flip Y so origin is top-left (matching X11 behaviour)
    loc.y = self.bounds.size.height - loc.y;
    return loc;
}

- (void)mouseDown:(NSEvent*)event {
    if (_onMouseDown && *_onMouseDown) {
        NSPoint loc = [self flippedLocation:event];
        (*_onMouseDown)((int)loc.x, (int)loc.y);
    }
}

- (void)mouseUp:(NSEvent*)event {
    if (_onMouseUp && *_onMouseUp) {
        NSPoint loc = [self flippedLocation:event];
        (*_onMouseUp)((int)loc.x, (int)loc.y);
    }
}

- (void)mouseMoved:(NSEvent*)event {
    if (_onMouseMove && *_onMouseMove) {
        NSPoint loc = [self flippedLocation:event];
        (*_onMouseMove)((int)loc.x, (int)loc.y);
    }
}

- (void)mouseDragged:(NSEvent*)event {
    if (_onMouseMove && *_onMouseMove) {
        NSPoint loc = [self flippedLocation:event];
        (*_onMouseMove)((int)loc.x, (int)loc.y);
    }
}

- (void)scrollWheel:(NSEvent*)event {
    if (_onMouseWheel && *_onMouseWheel) {
        float dx = (float)[event scrollingDeltaX];
        float dy = (float)[event scrollingDeltaY];
        // Negate dy to match X11 convention (scroll up = positive)
        (*_onMouseWheel)(-dx, dy);
    }
}

@end

@interface AppKitDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSWindow* window;
@property (nonatomic, copy) void (^frameBlock)(void);
@property (nonatomic, strong) CADisplayLink* displayLink;
@end

@implementation AppKitDelegate

- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    [_window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];

    if (_frameBlock) {
        _displayLink = [_window.contentView displayLinkWithTarget:self selector:@selector(render:)];
        [_displayLink addToRunLoop:[NSRunLoop mainRunLoop] forMode:NSRunLoopCommonModes];
    }
}

- (void)render:(CADisplayLink*)sender {
    if (_frameBlock) _frameBlock();
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
    [_displayLink invalidate];
    _displayLink = nil;
    return YES;
}
@end

class AppKitWindowImpl : public IWindow {
public:
    AppKitWindowImpl(int width, int height) : width_(width), height_(height) {
        NSRect frame = NSMakeRect(0, 0, width, height);
        view_ = [[EventView alloc] initWithFrame:frame];
        window_ = [[NSWindow alloc]
            initWithContentRect:frame
            styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
            backing:NSBackingStoreBuffered
            defer:NO];
        [window_ setContentView:view_];
        [window_ setAcceptsMouseMovedEvents:YES];
        [window_ center];

        // Add a tracking area so mouseMoved fires even when the window isn't key
        NSTrackingArea* trackingArea = [[NSTrackingArea alloc]
            initWithRect:frame
            options:NSTrackingMouseMoved | NSTrackingActiveAlways | NSTrackingInVisibleRect
            owner:view_
            userInfo:nil];
        [view_ addTrackingArea:trackingArea];

        // Wire event callbacks
        view_.onMouseDown  = &onMouseDown_;
        view_.onMouseUp    = &onMouseUp_;
        view_.onMouseMove  = &onMouseMove_;
        view_.onMouseWheel = &onMouseWheel_;

        delegate_ = [[AppKitDelegate alloc] init];
        delegate_.window = window_;
    }

    void run() override {
        [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
        [NSApp setDelegate:delegate_];

        // Wire the onFrame callback into the delegate so the timer can call it
        if (onFrame_) {
            auto frameCb = onFrame_;
            delegate_.frameBlock = ^{ frameCb(); };
        }

        [NSApp run];
    }

    void close() override {
        [window_ close];
    }

    int width()  const override { return width_; }
    int height() const override { return height_; }

    void resize(int w, int h) override {
        width_ = w;
        height_ = h;
        [window_ setContentSize:NSMakeSize(w, h)];
    }

    void setResizable(bool resizable) override {
        NSWindowStyleMask mask = [window_ styleMask];
        if (resizable)
            mask |= NSWindowStyleMaskResizable;
        else
            mask &= ~NSWindowStyleMaskResizable;
        [window_ setStyleMask:mask];
    }

    void* nativeHandle() const override {
        return (__bridge void*)view_;
    }

    void setOnMouseDown(std::function<void(int, int)> cb) override { onMouseDown_ = std::move(cb); }
    void setOnMouseUp  (std::function<void(int, int)> cb) override { onMouseUp_   = std::move(cb); }
    void setOnMouseMove(std::function<void(int, int)> cb) override { onMouseMove_ = std::move(cb); }
    void setOnMouseWheel(std::function<void(float, float)> cb) override { onMouseWheel_ = std::move(cb); }
    void setOnFrame    (std::function<void()> cb)          override { onFrame_     = std::move(cb); }
    void setOnClose    (std::function<void()> cb)          override { onClose_     = std::move(cb); }

private:
    NSWindow*       window_   = nil;
    EventView*      view_     = nil;
    AppKitDelegate* delegate_ = nil;
    int width_, height_;
    std::function<void(int, int)>     onMouseDown_;
    std::function<void(int, int)>     onMouseUp_;
    std::function<void(int, int)>     onMouseMove_;
    std::function<void(float, float)> onMouseWheel_;
    std::function<void()>             onFrame_;
    std::function<void()>             onClose_;
};

std::unique_ptr<IWindow> IWindow::createWindow(int w, int h, void* parentWindow) {
    return std::make_unique<AppKitWindowImpl>(w, h);
}
