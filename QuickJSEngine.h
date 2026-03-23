#include "IJSEngine.h"
#include <quickjs-libc.h>

class QuickJSEngine : public IJSEngine {
    public:
    QuickJSEngine();
    ~QuickJSEngine();

    void hotReload() override;
    void bindRenderer(IRenderer *renderer) override;
    void onMouseDown(float x, float y) override;

    // Methods
    static JSValue js_fillRect(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_strokeRect(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_roundRect(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_beginPath(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_arc(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_stroke(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_save(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_restore(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_fill(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_moveTo(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_lineTo(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_closePath(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_quadraticCurveTo(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_bezierCurveTo(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_arcTo(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_ellipse(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_rect(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);

    // Properties
    static JSValue js_fillStyle(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_strokeStyle(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_globalAlpha(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_lineWidth(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_lineCap(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_lineJoin(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);

    private:
    JSRuntime *rt;
    JSContext *ctx;
    IRenderer* currentRenderer = nullptr;
};
