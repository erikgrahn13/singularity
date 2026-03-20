#pragma once

#include "IRenderer.h"
#include <memory>

class IJSEngine {
    public:
    virtual void hotReload() = 0;
    virtual void bindRenderer(IRenderer* renderer) = 0;
};
