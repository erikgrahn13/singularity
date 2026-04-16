#pragma once

#include <functional>
#include <string>

class IWindow {
public:
    virtual ~IWindow() = default;
    virtual void run() = 0;
    virtual void close() = 0;
    virtual int width()  const = 0;
    virtual int height() const = 0;
    virtual void setOnMouseDown(std::function<void(int x, int y, unsigned int button)> cb) = 0;
    virtual void setOnMouseUp(std::function<void(int x, int y, unsigned int button)> cb)   = 0;
};
