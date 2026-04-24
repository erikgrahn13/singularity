#pragma once

#include "IRenderer2.h"
#include <visage/app.h>
#include <visage_graphics/path.h>
#include <string>
#include <vector>

class VisageRenderer : public IRenderer {
public:
    explicit VisageRenderer(void* parentHandle);

    // --- Component tree ---
    void* getRootComponent() override;
    void* createComponent(void* parentComponent) override;
    void setBounds(void* component, float x, float y, float w, float h) override;
    void setDrawCallback(void* component, std::function<void(void* canvas)> cb) override;
    void clear() override;
    void setComponentMouseDownCallback(std::function<void(void*, float, float)> cb) override;
    void setComponentMouseUpCallback(std::function<void(void*, float, float)> cb) override;
    void setComponentMouseDragCallback(std::function<void(void*, float, float)> cb) override;


    // --- Immediate rect helpers ---
    void fillRect(void* canvas, float x, float y, float width, float height) override;
    void strokeRect(void* canvas, float x, float y, float width, float height) override;
    void clearRect(void* canvas, float x, float y, float width, float height) override;

    // --- Path building ---
    void beginPath(void* canvas) override;
    void moveTo(void* canvas, float x, float y) override;
    void lineTo(void* canvas, float x, float y) override;
    void closePath(void* canvas) override;
    void arc(void* canvas, float cx, float cy, float radius,
             float startAngle, float endAngle, bool anticlockwise = false) override;
    void arcTo(void* canvas, float x1, float y1, float x2, float y2, float radius) override;
    void quadraticCurveTo(void* canvas, float cpx, float cpy, float x, float y) override;
    void bezierCurveTo(void* canvas, float cp1x, float cp1y,
                       float cp2x, float cp2y, float x, float y) override;
    void ellipse(void* canvas, float cx, float cy, float rx, float ry,
                 float rotation, float startAngle, float endAngle,
                 bool anticlockwise = false) override;
    void rect(void* canvas, float x, float y, float w, float h) override;
    void roundRect(void* canvas, float x, float y, float w, float h, float r) override;

    // --- Path rendering ---
    void fill(void* canvas) override;
    void stroke(void* canvas) override;

    // --- Text ---
    void fillText(void* canvas, const std::string& text, float x, float y) override;
    void strokeText(void* canvas, const std::string& text, float x, float y) override;
    float measureText(void* canvas, const std::string& text) override;

    // --- Style setters ---
    void setFillStyle(void* canvas, const std::string& color) override;
    void setStrokeStyle(void* canvas, const std::string& color) override;
    void setLineWidth(void* canvas, float width) override;
    void setFont(void* canvas, const std::string& font) override;
    void setGlobalAlpha(void* canvas, float alpha) override;
    void setTextAlign(void* canvas, const std::string& align) override;
    void setTextBaseline(void* canvas, const std::string& baseline) override;

    // --- State ---
    void save(void* canvas) override;
    void restore(void* canvas) override;

    // --- Transforms ---
    void translate(void* canvas, float x, float y) override;
    void rotate(void* canvas, float angle) override;
    void scale(void* canvas, float x, float y) override;
    void resetTransform(void* canvas) override;

private:
    visage::ApplicationWindow* rootFrame_{ nullptr };

    // Canvas drawing state (single-threaded; one active draw at a time)
    visage::Path currentPath_;

    struct DrawState {
        visage::Color fillColor{ 0xff000000u };
        visage::Color strokeColor{ 0xff000000u };
        float lineWidth{ 1.0f };
        float globalAlpha{ 1.0f };
        float fontSize{ 16.0f };
        std::string textAlign{ "left" };
        std::string textBaseline{ "alphabetic" };
        float translateX{ 0.0f };
        float translateY{ 0.0f };
        float scaleX{ 1.0f };
        float scaleY{ 1.0f };
    };

    DrawState state_;
    std::vector<DrawState> stateStack_;
    std::function<void(void*, float, float)> componentMouseDownCallback_;
    std::function<void(void*, float, float)> componentMouseUpCallback_;
    std::function<void(void*, float, float)> componentMouseDragCallback_;
};