#pragma once
#include <memory>
#include <functional>
#include <string>

class IRenderer {
public:
    static std::unique_ptr<IRenderer> createRenderer(void* parentHandle);

    struct PostEffectSpec {
        std::string type;
        float size = 0.0f;
        float intensity = 1.0f;
    };

    // --- Component tree ---
    virtual void* getRootComponent() = 0;
    virtual void* createComponent(void* parentComponent) = 0;
    virtual void setBounds(void* component, float x, float y, float w, float h) = 0;
    virtual void setDrawCallback(void* component, std::function<void(void* canvas)> cb) = 0;
    virtual void clear() = 0;
    virtual void setComponentMouseDownCallback(std::function<void(void*, float, float)> cb) = 0;
    virtual void setComponentMouseUpCallback(std::function<void(void*, float, float)> cb) = 0;
    virtual void setComponentMouseDragCallback(std::function<void(void*, float, float)> cb) = 0;
    virtual void redraw(void *component) = 0;
    virtual double getTime(void* canvas) = 0;
    virtual void setPostEffectForComponent(void* component, const PostEffectSpec& spec) = 0;
    


    // --- Immediate rect helpers ---
    virtual void fillRect(void* canvas, float x, float y, float width, float height) = 0;
    virtual void strokeRect(void* canvas, float x, float y, float width, float height) = 0;
    virtual void clearRect(void* canvas, float x, float y, float width, float height) = 0;

    // --- Path building ---
    virtual void beginPath(void* canvas) = 0;
    virtual void moveTo(void* canvas, float x, float y) = 0;
    virtual void lineTo(void* canvas, float x, float y) = 0;
    virtual void closePath(void* canvas) = 0;
    virtual void arc(void* canvas, float cx, float cy, float radius,
                     float startAngle, float endAngle, bool anticlockwise = false) = 0;
    virtual void arcTo(void* canvas, float x1, float y1, float x2, float y2, float radius) = 0;
    virtual void quadraticCurveTo(void* canvas, float cpx, float cpy, float x, float y) = 0;
    virtual void bezierCurveTo(void* canvas, float cp1x, float cp1y,
                                float cp2x, float cp2y, float x, float y) = 0;
    virtual void ellipse(void* canvas, float cx, float cy, float rx, float ry,
                         float rotation, float startAngle, float endAngle,
                         bool anticlockwise = false) = 0;
    virtual void rect(void* canvas, float x, float y, float w, float h) = 0;
    virtual void roundRect(void* canvas, float x, float y, float w, float h, float r) = 0;

    // --- Path rendering ---
    virtual void fill(void* canvas) = 0;
    virtual void stroke(void* canvas) = 0;

    // --- Text ---
    virtual void fillText(void* canvas, const std::string& text, float x, float y) = 0;
    virtual void strokeText(void* canvas, const std::string& text, float x, float y) = 0;
    virtual float measureText(void* canvas, const std::string& text) = 0;

    // --- Style setters ---
    virtual void setFillStyle(void* canvas, const std::string& color) = 0;
    virtual void setStrokeStyle(void* canvas, const std::string& color) = 0;
    virtual void setLineWidth(void* canvas, float width) = 0;
    virtual void setFont(void* canvas, const std::string& font) = 0;
    virtual void setGlobalAlpha(void* canvas, float alpha) = 0;
    virtual void setTextAlign(void* canvas, const std::string& align) = 0;
    virtual void setTextBaseline(void* canvas, const std::string& baseline) = 0;

    // --- State ---
    virtual void save(void* canvas) = 0;
    virtual void restore(void* canvas) = 0;

    // --- Transforms ---
    virtual void translate(void* canvas, float x, float y) = 0;
    virtual void rotate(void* canvas, float angle) = 0;
    virtual void scale(void* canvas, float x, float y) = 0;
    virtual void resetTransform(void* canvas) = 0;

    virtual ~IRenderer() = default;
};