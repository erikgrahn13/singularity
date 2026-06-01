#pragma once

#include <X11/Xlib.h>
#include <stdexcept>
#include <iostream>

#include "../IWindow.h"

class X11Window : public IWindow {
    public:

    X11Window(int width, int height);
    ~X11Window();

    void run() override;
    void close() override
    {

    }
    int width() const override  { return width_; }
    int height() const override { return height_; }

    void resize(int w, int h) override;
    void setOnMouseDown(std::function<void(int x, int y, unsigned int button)> cb) override
    {}
    void setOnMouseUp(std::function<void(int x, int y, unsigned int button)> cb) override
    {}
    void setOnMouseMove(std::function<void(int x, int y)> cb) override
    {}
    void setOnFrame(std::function<void()> cb) override
    { onFrame_ = std::move(cb); }

    Display* display() { return display_; }
    Window   xwindow() { return window_; }

    private:
    int      width_   = 0;
    int      height_  = 0;
    Display* display_{nullptr};
    Window   window_{0};
    std::function<void()> onFrame_;
};

