#pragma once

#include <functional>
#include <string>
#include "../IRenderer.h"

class IWindow {
public:
    virtual ~IWindow() = default;
    virtual void run() = 0;
    virtual void close() = 0;
    virtual int width()  const = 0;
    virtual int height() const = 0;
    virtual void setOnMouseDown(std::function<void(int x, int y, unsigned int button)> cb) = 0;
    virtual void setOnMouseUp(std::function<void(int x, int y, unsigned int button)> cb)   = 0;
    virtual void setOnMouseMove(std::function<void(int x, int y)> cb) = 0;
    // Called every frame. Return the pixel buffer to blit to the window.
    virtual void setOnFrame(std::function<DrawingContent()> cb) = 0;
    virtual void setOnClose(std::function<void()> cb) = 0;
    // Returns the underlying native window handle (HWND, NSWindow*, etc.)
    // Used by createNativeWindow to parent child windows.
    virtual void* nativeHandle() const { return nullptr; }
};
