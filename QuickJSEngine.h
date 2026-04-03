#include "IJSEngine.h"
#include <quickjs-libc.h>
#include <string>
#include <vector>
#include <unordered_map>
#include "IParameterProvider.h"

class QuickJSEngine : public IJSEngine {
    public:
    QuickJSEngine(IRenderer *renderer, IParameterProvider &parameterStore);
    ~QuickJSEngine();

    void hotReload() override;
    void renderUI() override;
    void onMouseDown(float x, float y) override;
    void onMouseUp(float x, float y) override;
    void onMouseMove(float x, float y) override;

    // Methods
    static JSValue js_fillRect(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_clearRect(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
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
    static JSValue js_fillText(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_strokeText(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_measureText(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_rotate(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_translate(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_scale(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_resetTransform(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_createLinearGradient(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_createRadialGradient(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_drawImage(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    
    // Properties
    static JSValue js_fillStyle(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_strokeStyle(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_globalAlpha(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_lineWidth(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_lineCap(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_lineJoin(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_font(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_textAlign(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_textBaseline(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_shadowColor(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_shadowBlur(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_shadowOffsetX(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_shadowOffsetY(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);

    // Audio parameter binding
    static JSValue js_setParameter(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_getParameter(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);

    // Event binding
    static JSValue js_addEventListener(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);

    private:
    void setupJS() override;
    void freeEventListeners();
    void dispatchEvent(const char* type, JSValue* args, int argc);
    static JSValue js_addColorStop(JSContext* ctx, JSValue this_val, int argc, JSValue* argv);
    JSRuntime *rt{nullptr};
    JSContext *ctx{nullptr};
    JSClassID gradientClassId = 0;
    IRenderer* currentRenderer = nullptr;
    IParameterProvider& parameterStore;
    std::unordered_map<std::string, std::vector<JSValue>> eventListeners;
};
