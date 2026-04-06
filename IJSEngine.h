#pragma once

#include "IRenderer.h"
#include <functional>
#include <memory>

class IJSEngine {
    public:
    virtual void hotReload() = 0;
    virtual void setupJS() = 0;
    virtual void renderUI() = 0;
    // Events
    virtual void onMouseDown(float x, float y) = 0;
    virtual void onMouseUp(float x, float y) = 0;
    virtual void onMouseMove(float x, float y) = 0;
    // Callbacks
    virtual void setOnOpenSettings(std::function<void()> cb) = 0;
};
