#pragma once

#include <vector>
#include "IRenderer.h"

#include "include/core/SkSurface.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPathBuilder.h"

class SkiaRenderer : public IRenderer {
    public:

    struct DrawState {
        // Colors
        SkColor fillStyle = SK_ColorBLACK;
        SkColor strokeStyle = SK_ColorBLACK;

        // Stroke properties
        float lineWidth = 1.0f;
        SkPaint::Cap lineCap = SkPaint::kButt_Cap;
        SkPaint::Join lineJoin = SkPaint::kMiter_Join;
        float miterLimit = 10.0f;
        
        // Transparency
        float globalAlpha = 1.0f;

        // Text
        std::string font = "10px sans-serif";
    };

    SkiaRenderer(int width, int height);
    
    void clear() override;

    void save() override;
    void restore() override;
    void setGlobalAlpha(float alpha) override;

    void setFillStyle(const std::string& color) override;
    void setStrokeStyle(const std::string &color) override;
    void setLineWidth(float lineWidth) override;
    void setLineCap(const std::string& cap) override;
    void setLineJoin(const std::string& join) override;


    void fillRect(float x, float y, float width, float height) override;
    void strokeRect(float x, float y, float width, float height) override;
    void roundRect(float x, float y, float width, float height, float radii) override;


    void beginPath() override;
    void stroke() override;
    void fill() override;
    void moveTo(float x, float y) override;


    void arc(float x, float y, float radius, float startAngle, float endAngle) override;
    
    
    DrawingContent getDrawingContent() override;
    int getWidth() const override;
    int getHeight() const override;
    
    private:

    // helper functions
    SkColor applyAlpha(SkColor color) const;


    SkPathBuilder     currentPath;
    sk_sp<SkSurface> skiaSurface;
    std::vector<DrawState> stateStack;
    DrawState currentState;
};
