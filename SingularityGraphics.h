#pragma once

#include "include/core/SkSurface.h"
#include "include/core/SkCanvas.h"
// #include <quickjs.h>  // already included by quickjs-libc.h
#include <quickjs-libc.h>
#include <iostream>
#include <string>

#ifndef JS_SCRIPTS_DIR
#  define JS_SCRIPTS_DIR ""
#endif

#if __has_include(<swift/bridging>)
#  include <swift/bridging>
#else
#  define SWIFT_RETURNS_INDEPENDENT_VALUE
#endif

class SingularityGraphicsWin;


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

    SWIFT_RETURNS_INDEPENDENT_VALUE const void* getPixels() const;
    int getWidth()       const;
    int getHeight()      const;
    size_t getRowBytes() const;

    static JSValue js_hello(JSContext* ctx, JSValue this_val, int argc, JSValue* argv);

    private:
    sk_sp<SkSurface> skiaSurface;
    JSRuntime* rt = nullptr;
    JSContext* ctx = nullptr;
};