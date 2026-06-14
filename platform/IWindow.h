#pragma once

#include <functional>
#include <string>
#include <memory>
#include "../IRenderer2.h"

class IWindow {
public:
    virtual ~IWindow() = default;
    virtual void run() = 0;
    virtual void close() = 0;
    virtual int width()  const = 0;
    virtual int height() const = 0;
    virtual void resize(int w, int h) = 0;
    virtual void setOnMouseDown(std::function<void(int x, int y)> cb) = 0;
    virtual void setOnMouseUp(std::function<void(int x, int y)> cb)   = 0;
    virtual void setOnMouseMove(std::function<void(int x, int y)> cb) = 0;
    virtual void setOnMouseWheel(std::function<void(float dx, float dy)> cb) {}
    virtual void setOnFrame(std::function<void()> cb) = 0;
    virtual void setOnClose(std::function<void()> cb) {}
    virtual void setResizable(bool resizable) {}
    // Returns the underlying native window handle (HWND, NSWindow*, etc.)
    // Used by createNativeWindow to parent child windows.
    virtual void* nativeHandle() const { return nullptr; }
    virtual int fd(){ return -1; }
    virtual void processEvents(){};
    virtual int refreshRate() const { return 60; }
    virtual void openFileDialog(const std::string& title,
                                std::function<void(const std::string&)> callback) {}

    static std::unique_ptr<IWindow> createWindow(int width, int height, void* parentWindow = nullptr);

};
