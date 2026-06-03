#include "AppKitWindow-Swift.h"
#include "../IWindow.h"
#include <memory>

// Swift C++ interop: the module name (SingularityMacWindow) becomes an outer namespace,
// and the class name (AppKitWindow) is nested inside it.
using SwiftAppKitWindow = SingularityMacWindow::AppKitWindow;

class AppKitWindowImpl : public IWindow {
public:
    AppKitWindowImpl(int width, int height)
        : swift_(SwiftAppKitWindow::init(width, height)), width_(width), height_(height) {}

    void run() override {
        swift_.runEventLoop();
    }

    void close() override {}

    int width()  const override { return width_; }
    int height() const override { return height_; }

    void resize(int w, int h) override {
        width_ = w;
        height_ = h;
    }

    void* nativeHandle() const override {
        return const_cast<AppKitWindowImpl*>(this)->swift_.nativeView();
    }

    void setOnMouseDown(std::function<void(int, int, unsigned int)> cb) override { onMouseDown_ = std::move(cb); }
    void setOnMouseUp  (std::function<void(int, int, unsigned int)> cb) override { onMouseUp_   = std::move(cb); }
    void setOnMouseMove(std::function<void(int, int)> cb)               override { onMouseMove_ = std::move(cb); }
    void setOnFrame    (std::function<void()> cb)                       override { onFrame_     = std::move(cb); }
    void setOnClose    (std::function<void()> cb)                       override { onClose_     = std::move(cb); }

private:
    SwiftAppKitWindow swift_;
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

