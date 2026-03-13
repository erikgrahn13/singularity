#pragma once

#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkPathBuilder.h"
#include "include/core/SkPaint.h"
// #include <quickjs.h>  // already included by quickjs-libc.h
#include <quickjs-libc.h>
#include <iostream>
#include <string>
#include <vector>

#include <functional>
#include <atomic>
#include "dmon.h"

#ifndef JS_SCRIPTS_DIR
#  define JS_SCRIPTS_DIR ""
#endif

#if __has_include(<swift/bridging>)
#  include <swift/bridging>
#else
#  define SWIFT_RETURNS_INDEPENDENT_VALUE
#endif

namespace Singularity {
    class Rect {
        public:
        Rect(float width, float height, float x, float y){
            left = x;
            top = y;
            right = x + width;
            bottom = y - height;
        }

        private:
        float left;
        float right;
        float bottom;
        float top;
    };
}

// class SingularityGraphics {

//     public:
//     SingularityGraphics();
//     void DrawRectangle();

//     // state
//     void setFillStyle(const char* color);
//     void setStrokeStyle(const char* color);
//     void setLineWidth(float width);
//     void setFont(const char* font);

//     // paths
//     void beginPath();
//     void moveTo(float x, float y);
//     void lineTo(float x, float y);
//     void arc(float x, float y, float radius, float startAngle, float endAngle);
//     void closePath();
//     void fill();
//     void stroke();

//     // rectangles
//     void fillRect(float x, float y, float w, float h);
//     void strokeRect(float x, float y, float w, float h);
//     void clearRect(float x, float y, float w, float h);

//     // text
//     void fillText(const char* text, float x, float y);
//     void strokeText(const char* text, float x, float y);



//     private:
//     sk_sp<SkSurface> skiaSurface;

//     friend class SingularityGraphicsWin;

// };

class SingularityGraphics {
    public:
    SingularityGraphics();
    SingularityGraphics(const std::string& jsPath);

    SingularityGraphics(const SingularityGraphics&) = delete;
    SingularityGraphics& operator=(const SingularityGraphics&) = delete;

    SingularityGraphics(SingularityGraphics&& other) noexcept;
    ~SingularityGraphics();

    void setOnFileChanged(std::function<void()> cb) { onFileChanged = std::move(cb); }
    void reloadScript();

    SWIFT_RETURNS_INDEPENDENT_VALUE const void* getPixels() const;
    int getWidth()    const;
    int getHeight()   const;
    size_t getRowBytes() const;

    // --- JS bindings ---
    static JSValue js_hello(JSContext*, JSValue, int, JSValue*);

    // rectangles
    static JSValue js_fillRect(JSContext*, JSValue, int, JSValue*);
    static JSValue js_strokeRect(JSContext*, JSValue, int, JSValue*);
    static JSValue js_clearRect(JSContext*, JSValue, int, JSValue*);

    // paths
    static JSValue js_beginPath(JSContext*, JSValue, int, JSValue*);
    static JSValue js_closePath(JSContext*, JSValue, int, JSValue*);
    static JSValue js_moveTo(JSContext*, JSValue, int, JSValue*);
    static JSValue js_lineTo(JSContext*, JSValue, int, JSValue*);
    static JSValue js_arc(JSContext*, JSValue, int, JSValue*);
    static JSValue js_bezierCurveTo(JSContext*, JSValue, int, JSValue*);
    static JSValue js_quadraticCurveTo(JSContext*, JSValue, int, JSValue*);
    static JSValue js_fill(JSContext*, JSValue, int, JSValue*);
    static JSValue js_stroke(JSContext*, JSValue, int, JSValue*);

    // transforms
    static JSValue js_save(JSContext*, JSValue, int, JSValue*);
    static JSValue js_restore(JSContext*, JSValue, int, JSValue*);
    static JSValue js_translate(JSContext*, JSValue, int, JSValue*);
    static JSValue js_rotate(JSContext*, JSValue, int, JSValue*);
    static JSValue js_scale(JSContext*, JSValue, int, JSValue*);

    // fillStyle
    static JSValue js_getFillStyle(JSContext*, JSValue, int, JSValue*);
    static JSValue js_setFillStyle(JSContext*, JSValue, int, JSValue*);

    // strokeStyle
    static JSValue js_getStrokeStyle(JSContext*, JSValue, int, JSValue*);
    static JSValue js_setStrokeStyle(JSContext*, JSValue, int, JSValue*);

    // lineWidth
    static JSValue js_getLineWidth(JSContext*, JSValue, int, JSValue*);
    static JSValue js_setLineWidth(JSContext*, JSValue, int, JSValue*);

    // globalAlpha
    static JSValue js_getGlobalAlpha(JSContext*, JSValue, int, JSValue*);
    static JSValue js_setGlobalAlpha(JSContext*, JSValue, int, JSValue*);

    // lineCap
    static JSValue js_getLineCap(JSContext*, JSValue, int, JSValue*);
    static JSValue js_setLineCap(JSContext*, JSValue, int, JSValue*);

    private:
    struct DrawState {
        SkColor fillStyle;
        SkColor strokeStyle;
        float   lineWidth;
        float   globalAlpha;
        SkPaint::Cap  lineCap;
        SkPaint::Join lineJoin;
    };

    // helpers
    SkPaint makeFillPaint()   const;
    SkPaint makeStrokePaint() const;

    // surface & JS runtime
    sk_sp<SkSurface> skiaSurface;
    JSRuntime* rt  = nullptr;
    JSContext* ctx = nullptr;

    // canvas state
    SkColor            fillStyle   = SK_ColorBLACK;
    SkColor            strokeStyle = SK_ColorBLACK;
    float              lineWidth   = 1.0f;
    float              globalAlpha = 1.0f;
    SkPaint::Cap       lineCap     = SkPaint::kRound_Cap;
    SkPaint::Join      lineJoin    = SkPaint::kRound_Join;
    SkPathBuilder     currentPath;
    std::vector<DrawState> stateStack;

    // hot reload
    std::string            jsPath;
    std::function<void()>  onFileChanged;
    std::atomic<bool>      scriptDirty { false };

    static void watch_callback(dmon_watch_id, dmon_action, const char*, const char*, const char*, void*);
};