#pragma once

#include "../IWindow.h"
#include <functional>
#include <memory>
#include <string>

// Opaque impl — hides NSWindow/AppKit types from C++ translation units
struct CocoaWindowImpl;

class CocoaWindow : public IWindow {
public:
    CocoaWindow(const std::string& title, int width, int height, IWindow* owner = nullptr);
    ~CocoaWindow() override;

    CocoaWindow(const CocoaWindow&) = delete;
    CocoaWindow& operator=(const CocoaWindow&) = delete;

    void run()   override;
    void close() override;
    int  width()  const override;
    int  height() const override;

    void setOnMouseDown(std::function<void(int, int, unsigned int)> cb) override;
    void setOnMouseUp  (std::function<void(int, int, unsigned int)> cb) override;
    void setOnMouseMove(std::function<void(int, int)> cb)               override;
    void setOnFrame    (std::function<DrawingContent()> cb)             override;
    void setOnClose    (std::function<void()> cb)                       override;

private:
    std::unique_ptr<CocoaWindowImpl> m_impl;
};
