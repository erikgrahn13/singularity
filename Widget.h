#pragma once
#include "SingularityGraphics.h"

class Widget {
    public:
    virtual void draw(SingularityGraphics& gfx) = 0;
    virtual ~Widget() = default;
};
