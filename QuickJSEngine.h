#include "IJSEngine.h"
#include <quickjs-libc.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include "IParameterProvider.h"

class QuickJSEngine : public IJSEngine {
    public:
    QuickJSEngine(IRenderer *renderer, IParameterProvider &parameterStore, bool standalone = false);
    ~QuickJSEngine();

    void hotReload() override;
    void renderUI() override;
    void onMouseDown(float x, float y) override;
    void onMouseUp(float x, float y) override;
    void onMouseMove(float x, float y) override;
    void setOnOpenSettings(std::function<void()> cb) override { onOpenSettings = std::move(cb); }
    void setOnSetBloom(std::function<void(float, float)> cb) override { onSetBloom = std::move(cb); }
    // void setOnRequestRedraw(std::function<void()> cb) override { onRequestRedraw = std::move(cb); }
    void setStringList(const std::string& key, std::vector<std::string> values) override { stringLists[key] = std::move(values); }

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

    // Settings modal window
    static JSValue js_openSettingsWindow(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_getAudioBackends(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_getStringList(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_setBloom(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);

    // Animation time
    static JSValue js_time(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_deltaTime(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_frameCount(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);

    // Visage-native GPU primitives
    static JSValue js_circle(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_fadeCircle(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_ring(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_squircle(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_roundedArc(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_flatArc(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_segment(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_triangle(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_diamond(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_setBlendMode(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_beginLayer(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_endLayer(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_requestRedraw(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_setLineDash(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_lineDashOffset(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);
    static JSValue js_hdrMultiplier(JSContext *ctx, JSValue this_val, int argc, JSValue* argv);

    private:
    void setupJS() override;
    void loadScript(const std::string& path) override;
    void freeEventListeners();
    void dispatchEvent(const char* type, JSValue* args, int argc);
    static JSValue js_addColorStop(JSContext* ctx, JSValue this_val, int argc, JSValue* argv);
    JSRuntime *rt{nullptr};
    JSContext *ctx{nullptr};
    JSClassID gradientClassId = 0;
    IRenderer* currentRenderer = nullptr;
    IParameterProvider& parameterStore;
    bool standalone = false;
    std::string currentScriptPath;
    std::unordered_map<std::string, std::vector<JSValue>> eventListeners;
    std::function<void()> onOpenSettings;
    std::function<void(float, float)> onSetBloom;
    // std::function<void()> onRequestRedraw;
    std::unordered_map<std::string, std::vector<std::string>> stringLists;
};
