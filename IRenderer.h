#pragma once

// #if __has_include(<swift/bridging>)
// #  include <swift/bridging>
// #else
// #  define SWIFT_SELF_CONTAINED
// #  define SWIFT_RETURNS_INDEPENDENT_VALUE
// #endif

#include <memory>
#include <string>
#include <vector>

namespace visage { class Canvas; }

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
    virtual void setLineDash(const std::vector<float>& segments) {}
    virtual void setLineDashOffset(float offset) {}
    virtual void setHdrMultiplier(float mult) {}
    virtual void setShadowColor(const std::string& color) = 0;
    virtual void setShadowBlur(float blur) = 0;
    virtual void setShadowOffsetX(float offsetX) = 0;
    virtual void setShadowOffsetY(float offsetY) = 0;
    virtual int createLinearGradient(float x0, float y0, float x1, float y1) = 0;
    virtual int createRadialGradient(float x0, float y0, float r0, float x1, float y1, float r1) = 0; 
    virtual void addColorStop(int id, float offset, const std::string& color) = 0;
    virtual void setFillStyleGradient(int i) = 0;
    virtual void setStrokeStyleGradient(int i) = 0;

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

    // Called once per frame before JS renderUI(). Resets accumulated per-frame
    // state (gradients, draw state, state stack) without clearing the canvas.
    virtual void resetFrame() {}

    // Receives the visage Canvas for the current frame. Called by SingularityGraphics
    // before renderUI(); VisageRenderer stores it, SkiaRenderer stores it for flush().
    virtual void setCanvas(visage::Canvas* /*canvas*/) {}

    // Called after JS renderUI(). SkiaRenderer blits its CPU pixels to the stored canvas.
    // VisageRenderer is a no-op — it drew directly during renderUI().
    virtual void flush() {}

    // Animation time in seconds. VisageRenderer forwards canvas.time(); others return 0.
    virtual double time()       const { return 0.0; }
    virtual double deltaTime()  const { return 0.0; }
    virtual int    frameCount() const { return 0; }

    // ---------------------------------------------------------------------------
    // Visage-native GPU primitives. Default no-op so SkiaRenderer ignores them.
    // Coordinates match canvas2D convention: x,y = top-left of bounding box.
    // ---------------------------------------------------------------------------
    // circle(cx, cy, radius) — center + radius
    virtual void circle(float cx, float cy, float radius) {}
    // fadeCircle — circle with a soft anti-aliased edge of pixelWidth pixels
    virtual void fadeCircle(float cx, float cy, float radius, float pixelWidth) {}
    // ring — donut; x,y = top-left of bounding box, diameter, ring thickness
    virtual void ring(float x, float y, float diameter, float thickness) {}
    // squircle — superellipse with adjustable power (4 = squircle, higher = more square)
    virtual void squircle(float x, float y, float diameter, float power) {}
    // roundedArc — arc with rounded end-caps; angles in radians, center_radians=12-o'clock=0
    virtual void roundedArc(float x, float y, float diameter, float thickness,
                            float centerRadians, float arcRadians) {}
    // flatArc — same but with flat (square) end-caps
    virtual void flatArc(float x, float y, float diameter, float thickness,
                         float centerRadians, float arcRadians) {}
    // segment — anti-aliased line segment between two points
    virtual void segment(float ax, float ay, float bx, float by, float thickness) {}
    // triangle — filled triangle
    virtual void triangle(float ax, float ay, float bx, float by, float cx, float cy) {}
    // diamond — rotated square
    virtual void diamond(float x, float y, float diameter) {}
    // setBlendMode — "alpha" (default), "add", "sub", "mult", "opaque", "composite", "maskAdd", "maskRemove"
    virtual void setBlendMode(const std::string& mode) {}
    // Canvas 2D Level 2 layer API — renders content into an isolated buffer, then
    // composites the whole buffer at 'opacity' onto the destination.
    virtual void beginLayer(float opacity) {}
    virtual void endLayer() {}

    virtual std::vector<uint8_t> encodeFrameToPng() = 0;

    virtual ~IRenderer() = default;
    virtual DrawingContent getDrawingContent() = 0; 
};
