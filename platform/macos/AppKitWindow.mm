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
        view_ = [[NSView alloc] initWithFrame:frame];
        window_ = [[NSWindow alloc]
            initWithContentRect:frame
            styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
            backing:NSBackingStoreBuffered
            defer:NO];
        [window_ setContentView:view_];
        [window_ center];

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
    void setOnMouseMove(std::function<void(int, int)> cb)               override { onMouseMove_ = std::move(cb); }
    void setOnFrame    (std::function<void()> cb)                       override { onFrame_     = std::move(cb); }
    void setOnClose    (std::function<void()> cb)                       override { onClose_     = std::move(cb); }

private:
    NSWindow*     window_   = nil;
    NSView*       view_     = nil;
    AppKitDelegate* delegate_ = nil;
    int width_, height_;
    std::function<void(int, int)> onMouseDown_;
    std::function<void(int, int)> onMouseUp_;
    std::function<void(int, int)>               onMouseMove_;
    std::function<void()>                       onFrame_;
    std::function<void()>                       onClose_;
};

std::unique_ptr<IWindow> IWindow::createWindow(int w, int h, void* parentWindow) {
    return std::make_unique<AppKitWindowImpl>(w, h);
}
