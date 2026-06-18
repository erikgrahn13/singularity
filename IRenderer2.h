#pragma once
#include <cstdint>
#include <memory>
#include <functional>
#include <string>
#include <string_view>

class IWindow;

class IRenderer {
public:
    static std::unique_ptr<IRenderer> createRenderer(std::string_view resourcePath);

    virtual void attachToWindow(IWindow& window) = 0;
    virtual void* beginFrame() = 0;  // returns SkCanvas*; nullptr = swapchain not ready
    virtual void* currentCanvas() const { return nullptr; }
    virtual void present() = 0;

    struct PostEffectSpec {
        std::string type;
        float size = 0.0f;
        float intensity = 1.0f;
    };

    // --- Surface size ---
    int width() const { return width_; }
    int height() const { return height_; }
    // virtual void setDrawCallback(void* component, std::function<void(void* canvas)> cb) = 0;
    // virtual void clear() = 0;
    // virtual void setComponentMouseDownCallback(std::function<void(void*, float, float)> cb) = 0;
    // virtual void setComponentMouseUpCallback(std::function<void(void*, float, float)> cb) = 0;
    // virtual void setComponentMouseDragCallback(std::function<void(void*, float, float)> cb) = 0;
    // virtual void setComponentMouseEnterCallback(std::function<void(void*)> cb) = 0;
    // virtual void setComponentMouseExitCallback(std::function<void(void*)> cb) = 0;
    // virtual void setComponentMouseWheelCallback(std::function<void(void*, float, float)> cb) = 0;
    // virtual void redraw(void *component) = 0;
    // virtual void redrawAll() = 0;
    // virtual double getTime(void* canvas) = 0;
    // virtual void setPostEffectForComponent(void* component, const PostEffectSpec& spec) = 0;
    


    // --- Immediate rect helpers ---
    virtual void fillRect(float x, float y, float width, float height) = 0;
    virtual void strokeRect(float x, float y, float width, float height) = 0;
    virtual void clearRect(float x, float y, float width, float height) = 0;

    // --- Path building ---
    virtual void beginPath(void* canvas) = 0;
    virtual void moveTo(float x, float y) = 0;
    virtual void lineTo(float x, float y) = 0;
    virtual void closePath(void* canvas) = 0;
    virtual void arc(float cx, float cy, float radius,
                     float startAngle, float endAngle, bool anticlockwise = false) = 0;
    virtual void arcTo(float x1, float y1, float x2, float y2, float radius) = 0;
    virtual void quadraticCurveTo(float cpx, float cpy, float x, float y) = 0;
    virtual void bezierCurveTo(float cp1x, float cp1y,
                                float cp2x, float cp2y, float x, float y) = 0;
    virtual void ellipse(float cx, float cy, float rx, float ry,
                         float rotation, float startAngle, float endAngle,
                         bool anticlockwise = false) = 0;
    virtual void rect(float x, float y, float w, float h) = 0;
    virtual void roundRect(float x, float y, float w, float h, float r) = 0;

    // --- Path rendering ---
    virtual void fill(void* canvas) = 0;
    virtual void stroke(void* canvas) = 0;

    // --- Text ---
    virtual void fillText(const std::string& text, float x, float y) = 0;
    virtual void strokeText(const std::string& text, float x, float y) = 0;
    virtual float measureText(const std::string& text) = 0;

    // --- Style setters ---
    virtual void setFillStyle(const std::string& color) = 0;
    virtual void setStrokeStyle(const std::string& color) = 0;
    virtual void setLineWidth(float width) = 0;
    virtual void setLineCap(const std::string& cap) = 0;
    virtual void setFont(const std::string& font) = 0;
    virtual void setGlobalAlpha(float alpha) = 0;
    virtual void setTextAlign(const std::string& align) = 0;
    virtual void setTextBaseline(const std::string& baseline) = 0;

    // --- Gradients ---
    virtual int createLinearGradient(float x0, float y0, float x1, float y1) = 0;
    virtual int createRadialGradient(float x0, float y0, float r0, float x1, float y1, float r1) = 0;
    virtual void addColorStop(int id, float offset, const std::string& color, float hdr = 1.0f) = 0;
    virtual void setFillStyleGradient(int id) = 0;
    virtual void setStrokeStyleGradient(int id) = 0;

    // --- State ---
    virtual void save(void* canvas) = 0;
    virtual void restore(void* canvas) = 0;

    // --- Transforms ---
    virtual void translate(float x, float y) = 0;
    virtual void rotate(float angle) = 0;
    virtual void scale(float x, float y) = 0;
    virtual void resetTransform(void* canvas) = 0;

    // --- Clipping ---
    virtual void clipRect(float x, float y, float w, float h) = 0;

    // --- Shadow / Glow ---
    virtual void setShadowColor(const std::string& color) = 0;
    virtual void setShadowBlur(float blur) = 0;
    virtual void setShadowOffsetX(float x) = 0;
    virtual void setShadowOffsetY(float y) = 0;

    // --- Bloom post-process ---
    // strength 0.0 = off, 1.0 = full bloom. Uses color(srgb-linear) values > 1.0 as sources.
    virtual void setBloom(float strength) {}

    // --- Time ---
    // Seconds elapsed since renderer creation; drives ctx.time() for animations.
    virtual double getTime() const { return 0.0; }

    // --- Layers (Canvas 2D Level 2 spec) ---
    virtual void beginLayer(float opacity) {}
    virtual void endLayer(void* canvas) {}

    virtual void drawImage(const std::string& name,
                       float dx, float dy, float dw, float dh) {}
    virtual void clearImageCache() {}
    virtual void registerImage(const std::string& name, const uint8_t* data, int size) {}

    virtual ~IRenderer() = default;

    void setOnResize(std::function<void(int, int)> cb) { onResize_ = std::move(cb); }

    virtual void resize(int w, int h) {
        if (w != width_ || h != height_) {
            width_ = w; height_ = h;
            if (onResize_) onResize_(w, h);
        }
    }

protected:
    int width_ = 0;
    int height_ = 0;
    std::function<void(int, int)> onResize_;
};