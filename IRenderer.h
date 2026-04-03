#pragma once

// #if __has_include(<swift/bridging>)
// #  include <swift/bridging>
// #else
// #  define SWIFT_SELF_CONTAINED
// #  define SWIFT_RETURNS_INDEPENDENT_VALUE
// #endif

#include <memory>
#include <string>

    struct DrawingContent {
        const void* contentAddres{nullptr};
        size_t contentBytes{0};
        int width{0};
        int height{0};
    };

class IRenderer {
    public:
    virtual void clear() = 0;

    virtual void renderBackground(float t) = 0;


    // Drawing State
    virtual void save() = 0;
    virtual void restore() = 0;
    virtual void setGlobalAlpha(float alpha) = 0;
    virtual void translate(float x, float y) = 0;
    virtual void rotate(float angle) = 0;
    virtual void scale(float x, float y) = 0;
    virtual void resetTransform() = 0;

    virtual void setFillStyle(const std::string& color) = 0;
    virtual void setStrokeStyle(const std::string& color) = 0;
    virtual void setLineWidth(float lineWidth) = 0;
    virtual void setLineCap(const std::string& cap) = 0;
    virtual void setLineJoin(const std::string& join) = 0;
    virtual void setShadowColor(const std::string& color) = 0;
    virtual void setShadowBlur(float blur) = 0;
    virtual void setShadowOffsetX(float offsetX) = 0;
    virtual void setShadowOffsetY(float offsetY) = 0;
    virtual int createLinearGradient(float x0, float y0, float x1, float y1) = 0;
    virtual int createRadialGradient(float x0, float y0, float r0, float x1, float y1, float r1) = 0; 
    virtual void addColorStop(int id, float offset, const std::string& color) = 0;
    virtual void setFillStyleGradient(int i) = 0;

    virtual void fillRect(float x, float y, float width, float height) = 0;
    virtual void clearRect(float x, float y, float width, float height) = 0;
    virtual void strokeRect(float x, float y, float width, float height) = 0;
    virtual void roundRect(float x, float y, float width, float height, float radii) = 0;
    virtual void beginPath() = 0;
    virtual void stroke() = 0;
    virtual void fill() = 0;
    virtual void moveTo(float x, float y) = 0;
    virtual void lineTo(float x, float y) = 0;
    virtual void closePath() = 0;
    virtual void quadraticCurveTo(float cpx, float cpy, float x, float y) = 0;
    virtual void bezierCurveTo(float cp1x, float cp1y, float cp2x, float cp2y, float x, float y) = 0;
    virtual void arcTo(float x1, float y1, float x2, float y2, float radius) = 0;
    virtual void ellipse(float x, float y, float radiusX, float radiusY, float rotation, float startAngle, float endAngle) = 0;
    virtual void rect(float x, float y, float width, float height) = 0;
    virtual void arc(float x, float y, float radius, float startAngle, float endAngle) = 0;

    virtual void fillText(const std::string& text, float x, float y) = 0;
    virtual void strokeText(const std::string& text, float x, float y) = 0;
    virtual float measureText(const std::string& text) = 0;
    virtual void font(const std::string& text) = 0;
    virtual void textAlign(const std::string& align) = 0;
    virtual void textBaseline(const std::string &baseline) = 0;

    virtual void registerImage(const std::string& name, const uint8_t *data, size_t size) = 0;
    virtual void drawImage(const std::string& name, float dx, float dy, float dw, float dh) = 0;

    virtual int getWidth() const = 0;
    virtual int getHeight() const = 0;

    virtual ~IRenderer() = default;
    virtual DrawingContent getDrawingContent() = 0; 
};
