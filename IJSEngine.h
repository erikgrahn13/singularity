#pragma once

#include "IRenderer.h"
#include <memory>

class IJSEngine {
    public:
    virtual void hotReload() = 0;
    virtual void setupJS() = 0;
    // Events
    virtual void onMouseDown(float x, float y) = 0;
};
