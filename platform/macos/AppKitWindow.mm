// AppKit window implementation using Objective-C++
#include "../IWindow.h"
#import <AppKit/AppKit.h>
#include <memory>

@interface AppKitDelegate : NSObject <NSApplicationDelegate>
@property (nonatomic, strong) NSWindow* window;
@end

@implementation AppKitDelegate
- (void)applicationDidFinishLaunching:(NSNotification*)notification {
    [_window makeKeyAndOrderFront:nil];
    [NSApp activateIgnoringOtherApps:YES];
}
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender {
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

    void setOnMouseDown(std::function<void(int, int, unsigned int)> cb) override { onMouseDown_ = std::move(cb); }
    void setOnMouseUp  (std::function<void(int, int, unsigned int)> cb) override { onMouseUp_   = std::move(cb); }
    void setOnMouseMove(std::function<void(int, int)> cb)               override { onMouseMove_ = std::move(cb); }
    void setOnFrame    (std::function<void()> cb)                       override { onFrame_     = std::move(cb); }
    void setOnClose    (std::function<void()> cb)                       override { onClose_     = std::move(cb); }

private:
    NSWindow*     window_   = nil;
    NSView*       view_     = nil;
    AppKitDelegate* delegate_ = nil;
    int width_, height_;
    std::function<void(int, int, unsigned int)> onMouseDown_;
    std::function<void(int, int, unsigned int)> onMouseUp_;
    std::function<void(int, int)>               onMouseMove_;
    std::function<void()>                       onFrame_;
    std::function<void()>                       onClose_;
};

std::unique_ptr<IWindow> IWindow::createWindow(int w, int h) {
    return std::make_unique<AppKitWindowImpl>(w, h);
}
