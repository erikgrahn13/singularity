#pragma once

#include "IRenderer.h"

#include "include/core/SkSurface.h"

class SkiaRenderer : public IRenderer {
    public:
    SkiaRenderer(int width, int height);

    DrawingContent getDrawingContent() override;

    private:
    sk_sp<SkSurface> skiaSurface;
};
