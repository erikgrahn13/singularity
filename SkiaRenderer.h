#pragma once

#include <vector>
#include "IRenderer.h"

#include "include/core/SkSurface.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPathBuilder.h"

class SkiaRenderer : public IRenderer {
    public:

    // struct DrawState {
    //     SkColor strokeStyle = SK_ColorBLACK;
    //     float lineWidth = 1.0f;
    //     float globalAlpha = 1.0f;
    //     SkPaint::Cap lineCap = SkPaint::kRound_Cap;
    //     SkPaint::Join lineJoin = SkPaint::kRound_Join;
    // };
    
    // DrawState currentState;
    // std::vector<DrawState> stateStack;
    
    SkiaRenderer(int width, int height);
    
    void clear() override;
    void setFillStyle(const std::string& color) override;
    void fillRect(float x, float y, float width, float height) override;
    void beginPath() override;
    void stroke() override;

    void arc(float x, float y, float radius, float startAngle, float endAngle) override;
    
    
    DrawingContent getDrawingContent() override;
    
    private:
    SkColor fillStyle = SK_ColorRED;
    SkPathBuilder     currentPath;
    sk_sp<SkSurface> skiaSurface;
};
