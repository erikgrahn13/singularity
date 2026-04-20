#pragma once

#include "IRenderer.h"

#include <visage_graphics/canvas.h>
#include <visage_graphics/color.h>
#include <visage_graphics/gradient.h>
#include <visage_graphics/path.h>
#include <visage_graphics/font.h>
#include <visage_graphics/region.h>

#include <map>
#include <string>
#include <vector>

class VisageRenderer : public IRenderer {
public:
    VisageRenderer(int width, int height);

    visage::Canvas* canvas() const { return canvas_; }

    // IRenderer
    void clear() override;
    void resetFrame() override;
    void renderBackground(float t) override {}

    void save() override;
    void restore() override;
    void setGlobalAlpha(float alpha) override;
    void translate(float x, float y) override;
    void rotate(float angle) override;
    void scale(float x, float y) override;
    void resetTransform() override;

    void setFillStyle(const std::string& color) override;
    void setStrokeStyle(const std::string& color) override;
    void setLineWidth(float lineWidth) override;
    void setLineCap(const std::string& cap) override;
    void setLineJoin(const std::string& join) override;
    void setLineDash(const std::vector<float>& segments) override;
    void setLineDashOffset(float offset) override;
    void setHdrMultiplier(float mult) override;
    void setShadowColor(const std::string& color) override {}
    void setShadowBlur(float blur) override {}
    void setShadowOffsetX(float offsetX) override {}
    void setShadowOffsetY(float offsetY) override {}

    int  createLinearGradient(float x0, float y0, float x1, float y1) override;
    int  createRadialGradient(float x0, float y0, float r0, float x1, float y1, float r1) override;
    void addColorStop(int id, float offset, const std::string& color) override;
    void setFillStyleGradient(int i) override;
    void setStrokeStyleGradient(int i) override;

    void fillRect(float x, float y, float width, float height) override;
    void clearRect(float x, float y, float width, float height) override;
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
    void ellipse(float x, float y, float radiusX, float radiusY, float rotation,
                 float startAngle, float endAngle) override;
    void rect(float x, float y, float width, float height) override;
    void arc(float x, float y, float radius, float startAngle, float endAngle) override;

    void fillText(const std::string& text, float x, float y) override;
    void strokeText(const std::string& text, float x, float y) override;
    float measureText(const std::string& text) override;
    void font(const std::string& text) override;
    void textAlign(const std::string& align) override;
    void textBaseline(const std::string& baseline) override;

    void registerImage(const std::string& name, const uint8_t* data, size_t size) override;
    void drawImage(const std::string& name, float dx, float dy, float dw, float dh) override;

    int  getWidth() const override { return width_; }
    int  getHeight() const override { return height_; }

    // Not used in visage path — no CPU pixel readback
    std::vector<uint8_t> encodeFrameToPng() override { return {}; }
    DrawingContent getDrawingContent() override { return {}; }

    void setCanvas(visage::Canvas* canvas) override { canvas_ = canvas; }

    // Animation time forwarded from visage canvas
    double time()       const override { return canvas_ ? canvas_->time()       : 0.0; }
    double deltaTime()  const override { return canvas_ ? canvas_->deltaTime()  : 0.0; }
    int    frameCount() const override { return canvas_ ? canvas_->frameCount() : 0;   }

    // Visage-native GPU primitives
    void circle(float cx, float cy, float radius) override;
    void fadeCircle(float cx, float cy, float radius, float pixelWidth) override;
    void ring(float x, float y, float diameter, float thickness) override;
    void squircle(float x, float y, float diameter, float power) override;
    void roundedArc(float x, float y, float diameter, float thickness,
                    float centerRadians, float arcRadians) override;
    void flatArc(float x, float y, float diameter, float thickness,
                 float centerRadians, float arcRadians) override;
    void segment(float ax, float ay, float bx, float by, float thickness) override;
    void triangle(float ax, float ay, float bx, float by, float cx, float cy) override;
    void diamond(float x, float y, float diameter) override;
    void setBlendMode(const std::string& mode) override;
    void beginLayer(float opacity) override;
    void endLayer() override;

private:
    struct GradientData {
        enum class Type { Linear, Radial } type = Type::Linear;
        float x0 = 0, y0 = 0, x1 = 0, y1 = 0;
        float r0 = 0, r1 = 0;
        std::vector<std::pair<float, visage::Color>> stops;
    };

    struct DrawState {
        visage::Color fillColor   { 0xffffffff };
        visage::Color strokeColor { 0xff000000 };
        float lineWidth      = 1.0f;
        float globalAlpha    = 1.0f;
        float translateX     = 0.0f;
        float translateY     = 0.0f;
        float scaleX         = 1.0f;
        float scaleY         = 1.0f;
        float rotateDeg      = 0.0f;
        int   fillGradientId   = -1;
        int   strokeGradientId = -1;
        std::string lineCap  = "butt";
        std::string lineJoin = "miter";
        std::vector<float> lineDash;
        float lineDashOffset   = 0.0f;
        float hdrMultiplier    = 1.0f;
        std::string textAlignStr    = "left";
        std::string textBaselineStr = "alphabetic";
        visage::BlendMode blendMode = visage::BlendMode::Alpha;
    };

    visage::Color parseColor(const std::string& color) const;
    visage::Brush makeFillBrush() const;
    visage::Brush makeStrokeBrush() const;
    void applyTransform();

    visage::Canvas*  canvas_ = nullptr;
    int width_;
    int height_;

    struct LayerState {
        float opacity = 1.0f;
    };

    DrawState state_;
    std::vector<DrawState> stateStack_;
    // Each entry is a persistent visage::Region registered with the canvas.
    // Regions are allocated on first use and reused every frame (like visage Frames).
    std::vector<std::unique_ptr<visage::Region>> layerPool_;
    std::vector<LayerState> layerStack_;

    visage::Path currentPath_;
    float fontSize_ = 14.0f;
    std::string fontFamily_;

    std::vector<GradientData> gradients_;

    // Registered images: name → PNG bytes kept alive
    std::map<std::string, std::vector<uint8_t>> registeredImages_;
};