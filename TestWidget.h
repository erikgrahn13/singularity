#pragma once

#include "Widget.h"

class TestWidget : public Widget {

    public:
    void draw(SingularityGraphics& gfx) override
    {
        gfx.DrawRectangle();
    }
};