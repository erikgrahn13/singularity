#pragma once

#include "IJSEngine.h"
#include "IParameterProvider.h"
#include "quickjs-libc.h"
#include <vector>
#include <unordered_map>

class QuickJSEngine : public IJSEngine {
    public:
    QuickJSEngine(IParameterProvider &parameterStore, Singularity::AudioDataExchange::AudioDataQueue* audioDataQueue = nullptr);
    void load(const std::string& entryFile, IRenderer* renderer) override;
    void draw() override;
    bool wantsAnimatedRedraw() const override { return hasAnimations_; }

    void installConsole();
    void log(const std::string& msg);

    void setLogger(LogCallback cb) override {
        logger_ = std::move(cb);
    }

    // Mouse events — hit-test against hitboxes_ collected during draw
    void onMouseDown(float x, float y) override;
    void onMouseUp(float x, float y) override;
    void onMouseMove(float x, float y) override;
    void onMouseWheel(float deltaX, float deltaY) override;

    JSValue getParameter(JSContext *ctx, JSValue this_val, int argc, JSValue *argv);
    JSValue setParameter(JSContext *ctx, JSValue this_val, int argc, JSValue *argv);
    JSValue getAudioData(JSContext *ctx, JSValue this_val, int argc, JSValue *argv);

    void setWindow(IWindow* window) { window_ = window; }
    IWindow* window() const { return window_; }

    JSValue appFn_ = JS_UNDEFINED;

    // Draw entry: absolute position + draw function, collected during buildComponentTree
    enum class DrawEntryType { Draw, ClipBegin, ClipEnd };
    struct DrawEntry {
        DrawEntryType type = DrawEntryType::Draw;
        float absX = 0, absY = 0;
        float width = 0, height = 0;
        std::string bgColor;        // if non-empty, fill this rect before calling fn
        float borderRadius = 0.f;   // rounded corners
        std::string borderColor;    // if non-empty, stroke a border
        float borderWidth  = 1.f;
        JSValue fn = JS_UNDEFINED;
    };
    std::vector<DrawEntry> drawEntries_;
    std::string backgroundColor_;

    // Hitbox entry built during buildComponentTree each frame
    struct Hitbox {
        float x, y, w, h;
        JSValue onMouseDown  = JS_UNDEFINED;
        JSValue onMouseUp    = JS_UNDEFINED;
        JSValue onMouseDrag  = JS_UNDEFINED;
        JSValue onMouseWheel = JS_UNDEFINED;
    };
    std::vector<Hitbox> hitboxes_; // rebuilt each frame, not ref-counted (borrowed from live JS tree)
    bool hasAnimations_ = false;   // true if any component declares animate:true

    private:
    // Helpers
    void callJSMouseHandler(JSValue fn, float x, float y);
    void callJSMouseWheelHandler(JSValue fn, float dx, float dy);
    void createCanvasContext();
    Hitbox* hitTest(float x, float y);

    JSRuntime* rt_{nullptr};
    JSContext* ctx_{nullptr};
    IRenderer* renderer_{nullptr};
    JSValue jsCanvasCtx_ = JS_UNDEFINED; // persistent canvas context object, reused every frame

    // Drag state — which hitbox's handlers are active for the current press
    JSValue activeDragFn_    = JS_UNDEFINED;
    JSValue activeMouseUpFn_ = JS_UNDEFINED;
    bool    dragging_        = false;
    float   lastMouseX_      = 0, lastMouseY_ = 0;
    float   dragOffsetX_     = 0, dragOffsetY_ = 0; // hitbox origin at drag start

    IParameterProvider& parameterStore_;
    Singularity::AudioDataExchange::AudioDataQueue* audioDataQueue_ = nullptr;
    Singularity::AudioDataExchange::AudioDataBlock latestAudioData_{};
    std::uint32_t audioDataRevision_ = 0;
    IWindow* window_{nullptr};
};