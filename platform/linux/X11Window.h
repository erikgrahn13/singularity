#pragma once

#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>
#include <stdexcept>
#include <iostream>
#include <optional>
#include <string>
#include <sys/types.h>

#include "../IWindow.h"

class X11Window : public IWindow {
    public:

    X11Window(int width, int height);
    X11Window(int width, int height, void* parentWindow);
    ~X11Window();

    void run() override;
    void close() override
    {

    }
    int width() const override  { return width_; }
    int height() const override { return height_; }

    void resize(int w, int h) override;
    void setResizable(bool resizable) override;
    void setOnMouseDown(std::function<void(int x, int y)> cb) override
    { onMouseDown_ = std::move(cb); }
    void setOnMouseUp(std::function<void(int x, int y)> cb) override
    { onMouseUp_ = std::move(cb); }
    void setOnMouseMove(std::function<void(int x, int y)> cb) override
    { onMouseMove_ = std::move(cb); }
    void setOnMouseWheel(std::function<void(float dx, float dy)> cb) override
    { onMouseWheel_ = std::move(cb); }
    void setOnFrame(std::function<void()> cb) override
    { onFrame_ = std::move(cb); }

    Display* display() { return display_; }
    Window   xwindow() { return window_; }
    int fd() override { return ConnectionNumber(display_); };
    void processEvents() override;
    int refreshRate() const override;
    void openFileDialog(const std::string& title,
                        std::function<void(const std::string&)> callback) override;
    void openDirectoryDialog(const std::string& title,
                             std::function<void(const std::string&)> callback) override;

    private:
    struct ChooserProcess
    {
        pid_t pid = -1;
        int outputFd = -1;
        std::string output;
        std::function<void(const std::string&)> callback;
    };

    void launchChooser(const std::string& title, bool selectDirectory,
                       std::function<void(const std::string&)> callback);
    void pollChooser();
    void cancelChooser();

    int      width_   = 0;
    int      height_  = 0;
    Display* display_{nullptr};
    Window   window_{0};
    std::function<void()> onFrame_;
    std::function<void(int, int)> onMouseDown_;
    std::function<void(int, int)> onMouseUp_;
    std::function<void(int, int)> onMouseMove_;
    std::function<void(float, float)> onMouseWheel_;
    std::optional<ChooserProcess> chooserProcess_;
};
