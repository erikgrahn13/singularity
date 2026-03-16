#pragma once

#include "IRenderer.h"

#include "include/core/SkSurface.h"

class SkiaRenderer : public IRenderer {
    public:
    SkiaRenderer();

    DrawingContent getDrawingContent() override;

    private:
    sk_sp<SkSurface> skiaSurface;
};
