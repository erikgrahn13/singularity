#pragma once

#include <vector>
#include "IRenderer.h"

#include "include/core/SkSurface.h"
#include "include/core/SkTypeface.h"
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
        float fontSize = 10.0f;
        std::string fontFamily = "";
        std::string textAlign = "start";
        SkFontStyle fontStyle = SkFontStyle();
        // std::string font = "10px sans-serif";
    };

    SkiaRenderer(int width, int height);
    void renderBackground(float t);

    
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
    void lineTo(float x, float y) override;
    void closePath() override;
    void quadraticCurveTo(float cpx, float cpy, float x, float y) override;
    void bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y) override;
    void arcTo(float x1, float y1, float x2, float y2, float radius) override;
    void ellipse(float x, float y, float radiusX, float radiusY, float rotation, float startAngle, float endAngle) override;
    void rect(float x, float y, float width, float height) override;
    void arc(float x, float y, float radius, float startAngle, float endAngle) override;
    void fillText(const std::string& text, float x, float y) override;
    void strokeText(const std::string& text, float x, float y) override;
    float measureText(const std::string& text) override;
    void font(const std::string& text) override;
    void textAlign(const std::string& align) override;


    
    
    DrawingContent getDrawingContent() override;
    int getWidth() const override;
    int getHeight() const override;
    
    private:

    // helper functions
    SkColor applyAlpha(SkColor color) const;


    SkPathBuilder     currentPath;
    sk_sp<SkSurface> skiaSurface;
    sk_sp<SkTypeface> defaultTypeface;
    sk_sp<SkFontMgr> fontMgr;
    std::vector<DrawState> stateStack;
    DrawState currentState;
};
