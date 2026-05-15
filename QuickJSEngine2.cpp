#include "QuickJSEngine2.h"
#include "IJSEngine.h"
#include <iostream>
#include "QuickJSEngineCanvasAPI.h"

#if NDEBUG
#include "generated.h"
#endif

std::unique_ptr<IJSEngine> IJSEngine::createJSEngine(IParameterProvider &parameterStore)
{
    return std::make_unique<QuickJSEngine>(parameterStore);
}

static JSValue js_mount(JSContext* ctx, JSValueConst this_val,
                        int argc, JSValueConst* argv) {
    if (argc < 1 || !JS_IsFunction(ctx, argv[0]))
        return JS_ThrowTypeError(ctx, "mount expects a function");

    auto* engine = static_cast<QuickJSEngine*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
    if (!JS_IsUndefined(engine->appFn_))
        JS_FreeValue(ctx, engine->appFn_);

    engine->appFn_  = JS_DupValue(ctx, argv[0]);

    return JS_UNDEFINED;
}

static JSValue js_console_log(JSContext* ctx, JSValueConst this_val,
                             int argc, JSValueConst* argv)
{
    auto* engine = static_cast<QuickJSEngine*>(
        JS_GetRuntimeOpaque(JS_GetRuntime(ctx))
    );

    std::string msg;

    for (int i = 0; i < argc; ++i) {
        const char* str = JS_ToCString(ctx, argv[i]);
        if (str) {
            msg += str;
            JS_FreeCString(ctx, str);
        }

        if (i + 1 < argc)
            msg += " ";
    }

    if (engine) {
        engine->log(msg);
    }

    return JS_UNDEFINED;
}

static JSValue js_Component(JSContext* ctx, JSValueConst this_val,
                            int argc, JSValueConst* argv) {
    JSValue obj = JS_NewObject(ctx);

    // type: "Component"
    JS_SetPropertyStr(ctx, obj, "type", JS_NewString(ctx, "Component"));

    // props: first argument or {}
    JSValue props = (argc > 0) ? JS_DupValue(ctx, argv[0]) : JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "props", props);

    return obj;
}

static JSValue js_getParameter(JSContext* ctx, JSValueConst this_val,
                               int argc, JSValueConst* argv)
{
    auto* engine = static_cast<QuickJSEngine*>(
        JS_GetRuntimeOpaque(JS_GetRuntime(ctx))
    );

    return engine->getParameter(ctx, this_val, argc, argv);
}

static JSValue js_setParameter(JSContext* ctx, JSValueConst this_val,
                               int argc, JSValueConst* argv)
{
    auto* engine = static_cast<QuickJSEngine*>(
        JS_GetRuntimeOpaque(JS_GetRuntime(ctx))
    );

    return engine->setParameter(ctx, this_val, argc, argv);
}

static const JSCFunctionListEntry singularity_funcs[] = {
    JS_CFUNC_DEF("Component", 1, js_Component),
    JS_CFUNC_DEF("mount", 1, js_mount),
    JS_CFUNC_DEF("getParameter", 1, js_getParameter),
    JS_CFUNC_DEF("setParameter", 2, js_setParameter),
    // JS_CFUNC_DEF("on", 2, js_on),
};

static int js_singularity_init(JSContext* ctx, JSModuleDef* m) {
    return JS_SetModuleExportList(ctx, m, singularity_funcs,
                                  sizeof(singularity_funcs) / sizeof(JSCFunctionListEntry));
}

extern "C" JSModuleDef* js_init_module_singularity(JSContext* ctx, const char* module_name) {
    JSModuleDef* m = JS_NewCModule(ctx, module_name, js_singularity_init);
    if (!m)
        return nullptr;

    JS_AddModuleExportList(ctx, m, singularity_funcs,
                           sizeof(singularity_funcs) / sizeof(JSCFunctionListEntry));
    return m;
}

void QuickJSEngine::installConsole()
{
    JSValue global = JS_GetGlobalObject(ctx_);

    JSValue console = JS_NewObject(ctx_);

    JS_SetPropertyStr(ctx_, console, "log",
        JS_NewCFunction(ctx_, js_console_log, "log", 1));

    JS_SetPropertyStr(ctx_, global, "console", console);

    JS_FreeValue(ctx_, global);
}

int getIntProp(JSContext* ctx, JSValueConst props, const char* name, int fallback = 0) {
    JSValue val = JS_GetPropertyStr(ctx, props, name);

    int32_t out = fallback;
    if (!JS_IsUndefined(val) && !JS_IsNull(val)) {
        JS_ToInt32(ctx, &out, val);
    }

    JS_FreeValue(ctx, val);
    return out;
}

void applyPropsToFrame(JSContext* ctx, JSValueConst props, IRenderer* renderer, void* component) {
    int x = getIntProp(ctx, props, "x", 0);
    int y = getIntProp(ctx, props, "y", 0);
    int width = getIntProp(ctx, props, "width", -1);
    int height = getIntProp(ctx, props, "height", -1);

    if (width != -1 && height != -1) {
        renderer->setBounds(component, (float)x, (float)y, (float)width, (float)height);
    }

 // TEST BLOOM
JSValue postEffectProp = JS_GetPropertyStr(ctx, props, "postEffect");
if (JS_IsObject(postEffectProp)) {
    IRenderer::PostEffectSpec spec;
    JSValue typeVal = JS_GetPropertyStr(ctx, postEffectProp, "type");
    if (JS_IsString(typeVal)) {
        const char* typeStr = JS_ToCString(ctx, typeVal);
        if (typeStr) spec.type = typeStr;
        JS_FreeCString(ctx, typeStr);
    }
    JS_FreeValue(ctx, typeVal);

    JSValue sizeVal = JS_GetPropertyStr(ctx, postEffectProp, "size");
    if (JS_IsNumber(sizeVal)) {
        double size;
        JS_ToFloat64(ctx, &size, sizeVal);
        spec.size = static_cast<float>(size);
    }
    JS_FreeValue(ctx, sizeVal);

    JSValue intensityVal = JS_GetPropertyStr(ctx, postEffectProp, "intensity");
    if (JS_IsNumber(intensityVal)) {
        double intensity;
        JS_ToFloat64(ctx, &intensity, intensityVal);
        spec.intensity = static_cast<float>(intensity);
    }
    JS_FreeValue(ctx, intensityVal);

    renderer->setPostEffectForComponent(component, spec);
}
JS_FreeValue(ctx, postEffectProp);

//


    std::string backgroundColor;
    JSValue bgColorVal = JS_GetPropertyStr(ctx, props, "backgroundColor");
    if (JS_IsString(bgColorVal)) {
        const char* bgStr = JS_ToCString(ctx, bgColorVal);
        if (bgStr) { backgroundColor = bgStr; JS_FreeCString(ctx, bgStr); }
    }
    JS_FreeValue(ctx, bgColorVal);

    JSValue draw = JS_GetPropertyStr(ctx, props, "draw");

    if (JS_IsFunction(ctx, draw) || !backgroundColor.empty()) {
        auto* engine = static_cast<QuickJSEngine*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
        JSValue drawFn = JS_IsFunction(ctx, draw) ? JS_DupValue(ctx, draw) : JS_UNDEFINED;
        if (!JS_IsUndefined(drawFn)) engine->drawCallbacks_.push_back(drawFn);
        renderer->setDrawCallback(component, [ctx, drawFn, renderer, component, backgroundColor, width, height](void* canvas) mutable {
            if (!backgroundColor.empty()) {
                renderer->setFillStyle(canvas, backgroundColor);
                renderer->fillRect(canvas, 0, 0, (float)width, (float)height);
            }

            DrawContextData drawData;
            drawData.renderer = renderer;
            drawData.canvas = canvas;
            drawData.component = component; // Store the component (Frame/ApplicationWindow)

            void* previousOpaque = JS_GetContextOpaque(ctx);
            JS_SetContextOpaque(ctx, &drawData);

            JSValue jsCtx = JS_NewObject(ctx);

            // CANVAS API FUNCTIONS
            JS_SetPropertyStr(ctx, jsCtx, "fillRect",           JS_NewCFunction(ctx, js_fillRect,           "fillRect",           4));
            JS_SetPropertyStr(ctx, jsCtx, "strokeRect",         JS_NewCFunction(ctx, js_strokeRect,         "strokeRect",         4));
            JS_SetPropertyStr(ctx, jsCtx, "clearRect",          JS_NewCFunction(ctx, js_clearRect,          "clearRect",          4));
            JS_SetPropertyStr(ctx, jsCtx, "beginPath",          JS_NewCFunction(ctx, js_beginPath,          "beginPath",          0));
            JS_SetPropertyStr(ctx, jsCtx, "closePath",          JS_NewCFunction(ctx, js_closePath,          "closePath",          0));
            JS_SetPropertyStr(ctx, jsCtx, "moveTo",             JS_NewCFunction(ctx, js_moveTo,             "moveTo",             2));
            JS_SetPropertyStr(ctx, jsCtx, "lineTo",             JS_NewCFunction(ctx, js_lineTo,             "lineTo",             2));
            JS_SetPropertyStr(ctx, jsCtx, "arc",                JS_NewCFunction(ctx, js_arc,                "arc",                5));
            JS_SetPropertyStr(ctx, jsCtx, "arcTo",              JS_NewCFunction(ctx, js_arcTo,              "arcTo",              5));
            JS_SetPropertyStr(ctx, jsCtx, "quadraticCurveTo",   JS_NewCFunction(ctx, js_quadraticCurveTo,   "quadraticCurveTo",   4));
            JS_SetPropertyStr(ctx, jsCtx, "bezierCurveTo",      JS_NewCFunction(ctx, js_bezierCurveTo,      "bezierCurveTo",      6));
            JS_SetPropertyStr(ctx, jsCtx, "ellipse",            JS_NewCFunction(ctx, js_ellipse,            "ellipse",            7));
            JS_SetPropertyStr(ctx, jsCtx, "rect",               JS_NewCFunction(ctx, js_rect,               "rect",               4));
            JS_SetPropertyStr(ctx, jsCtx, "roundRect",          JS_NewCFunction(ctx, js_roundRect,          "roundRect",          5));
            JS_SetPropertyStr(ctx, jsCtx, "fill",               JS_NewCFunction(ctx, js_fill,               "fill",               0));
            JS_SetPropertyStr(ctx, jsCtx, "stroke",             JS_NewCFunction(ctx, js_stroke,             "stroke",             0));
            JS_SetPropertyStr(ctx, jsCtx, "fillText",           JS_NewCFunction(ctx, js_fillText,           "fillText",           3));
            JS_SetPropertyStr(ctx, jsCtx, "strokeText",         JS_NewCFunction(ctx, js_strokeText,         "strokeText",         3));
            JS_SetPropertyStr(ctx, jsCtx, "measureText",        JS_NewCFunction(ctx, js_measureText,        "measureText",        1));
            JS_SetPropertyStr(ctx, jsCtx, "save",               JS_NewCFunction(ctx, js_save,               "save",               0));
            JS_SetPropertyStr(ctx, jsCtx, "restore",            JS_NewCFunction(ctx, js_restore,            "restore",            0));
            JS_SetPropertyStr(ctx, jsCtx, "translate",          JS_NewCFunction(ctx, js_translate,          "translate",          2));
            JS_SetPropertyStr(ctx, jsCtx, "rotate",             JS_NewCFunction(ctx, js_rotate,             "rotate",             1));
            JS_SetPropertyStr(ctx, jsCtx, "scale",              JS_NewCFunction(ctx, js_scale,              "scale",              2));
            JS_SetPropertyStr(ctx, jsCtx, "resetTransform",     JS_NewCFunction(ctx, js_resetTransform,     "resetTransform",     0));
            JS_SetPropertyStr(ctx, jsCtx, "createLinearGradient", JS_NewCFunction(ctx, js_createLinearGradient, "createLinearGradient", 4));
            JS_SetPropertyStr(ctx, jsCtx, "createRadialGradient", JS_NewCFunction(ctx, js_createRadialGradient, "createRadialGradient", 6));

            // Add ctx.time() for animation
            auto js_time = [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                void* opaque = JS_GetContextOpaque(ctx);
                if (!opaque) return JS_NewFloat64(ctx, 0.0);
                DrawContextData* drawData = static_cast<DrawContextData*>(opaque);
                if (!drawData || !drawData->renderer || !drawData->canvas) return JS_NewFloat64(ctx, 0.0);
                double t = drawData->renderer->getTime(drawData->canvas);
                return JS_NewFloat64(ctx, t);
            };
            JS_SetPropertyStr(ctx, jsCtx, "time", JS_NewCFunction(ctx, js_time, "time", 0));

            // Add ctx.redraw() to allow JS to request a new frame (visage-style)
            auto js_redraw = [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                void* opaque = JS_GetContextOpaque(ctx);
                if (!opaque) return JS_UNDEFINED;
                DrawContextData* drawData = static_cast<DrawContextData*>(opaque);
                if (!drawData || !drawData->renderer || !drawData->component) return JS_UNDEFINED;
                drawData->renderer->redraw(drawData->component);
                return JS_UNDEFINED;
            };
            JS_SetPropertyStr(ctx, jsCtx, "redraw", JS_NewCFunction(ctx, js_redraw, "redraw", 0));

            // ctx.beginLayer({ opacity }) — Canvas 2D Level 2: composite children into an isolated layer
            auto js_beginLayer = [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                void* opaque = JS_GetContextOpaque(ctx);
                if (!opaque) return JS_UNDEFINED;
                DrawContextData* d = static_cast<DrawContextData*>(opaque);
                float opacity = 1.0f;
                if (argc > 0 && JS_IsObject(argv[0])) {
                    JSValue opVal = JS_GetPropertyStr(ctx, argv[0], "opacity");
                    if (!JS_IsUndefined(opVal)) {
                        double v; JS_ToFloat64(ctx, &v, opVal); opacity = (float)v;
                    }
                    JS_FreeValue(ctx, opVal);
                } else if (argc > 0 && JS_IsNumber(argv[0])) {
                    double v; JS_ToFloat64(ctx, &v, argv[0]); opacity = (float)v;
                }
                d->renderer->beginLayer(d->canvas, opacity);
                return JS_UNDEFINED;
            };
            JS_SetPropertyStr(ctx, jsCtx, "beginLayer", JS_NewCFunction(ctx, js_beginLayer, "beginLayer", 1));

            // ctx.endLayer() — Canvas 2D Level 2
            auto js_endLayer = [](JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv) -> JSValue {
                void* opaque = JS_GetContextOpaque(ctx);
                if (!opaque) return JS_UNDEFINED;
                DrawContextData* d = static_cast<DrawContextData*>(opaque);
                d->renderer->endLayer(d->canvas);
                return JS_UNDEFINED;
            };
            JS_SetPropertyStr(ctx, jsCtx, "endLayer", JS_NewCFunction(ctx, js_endLayer, "endLayer", 0));

            // CANVAS API PROPERTIES (setters)
            JS_DefinePropertyGetSet(ctx, jsCtx, JS_NewAtom(ctx, "fillStyle"),                JS_UNDEFINED, JS_NewCFunction(ctx, js_fillStyle,                "fillStyle",                1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
            JS_DefinePropertyGetSet(ctx, jsCtx, JS_NewAtom(ctx, "strokeStyle"),              JS_UNDEFINED, JS_NewCFunction(ctx, js_strokeStyle,              "strokeStyle",              1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
            JS_DefinePropertyGetSet(ctx, jsCtx, JS_NewAtom(ctx, "lineWidth"),                JS_UNDEFINED, JS_NewCFunction(ctx, js_lineWidth,                "lineWidth",                1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
            JS_DefinePropertyGetSet(ctx, jsCtx, JS_NewAtom(ctx, "font"),                     JS_UNDEFINED, JS_NewCFunction(ctx, js_font,                     "font",                     1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
            JS_DefinePropertyGetSet(ctx, jsCtx, JS_NewAtom(ctx, "globalAlpha"),              JS_UNDEFINED, JS_NewCFunction(ctx, js_globalAlpha,              "globalAlpha",              1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
            JS_DefinePropertyGetSet(ctx, jsCtx, JS_NewAtom(ctx, "textAlign"),                JS_UNDEFINED, JS_NewCFunction(ctx, js_textAlign,                "textAlign",                1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
            JS_DefinePropertyGetSet(ctx, jsCtx, JS_NewAtom(ctx, "textBaseline"),             JS_UNDEFINED, JS_NewCFunction(ctx, js_textBaseline,             "textBaseline",             1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
            JS_DefinePropertyGetSet(ctx, jsCtx, JS_NewAtom(ctx, "hdrMultiplier"),             JS_UNDEFINED, JS_NewCFunction(ctx, js_hdrMultiplier,             "hdrMultiplier",             1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);

            if (JS_IsFunction(ctx, drawFn)) {
                JSValue argv[1] = { jsCtx };
                JSValue result = JS_Call(ctx, drawFn, JS_UNDEFINED, 1, argv);

                if (JS_IsException(result))
                    js_std_dump_error(ctx);

                JS_FreeValue(ctx, result);
            }
            JS_FreeValue(ctx, jsCtx);

            JS_SetContextOpaque(ctx, previousOpaque);
        });
    }
    JS_FreeValue(ctx, draw);

    JSValue onMouseDown = JS_GetPropertyStr(ctx, props, "onMouseDown");
    if (JS_IsFunction(ctx, onMouseDown)) {
        auto* engine = static_cast<QuickJSEngine*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
        engine->registerMouseDownHandler(component, ctx, onMouseDown);
    }
    JS_FreeValue(ctx, onMouseDown);

    JSValue onMouseUp = JS_GetPropertyStr(ctx, props, "onMouseUp");
    if (JS_IsFunction(ctx, onMouseUp)) {
        auto* engine = static_cast<QuickJSEngine*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
        engine->registerMouseUpHandler(component, ctx, onMouseUp);
    }
    JS_FreeValue(ctx, onMouseUp);

    JSValue onMouseDrag = JS_GetPropertyStr(ctx, props, "onMouseDrag");

    if (JS_IsFunction(ctx, onMouseDrag)) {
        auto* engine = static_cast<QuickJSEngine*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
        engine->registerMouseDragHandler(component, ctx, onMouseDrag);
    }

    JS_FreeValue(ctx, onMouseDrag);

    JSValue onMouseEnter = JS_GetPropertyStr(ctx, props, "onMouseEnter");
    if (JS_IsFunction(ctx, onMouseEnter)) {
        auto* engine = static_cast<QuickJSEngine*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
        engine->registerMouseEnterHandler(component, ctx, onMouseEnter);
    }
    JS_FreeValue(ctx, onMouseEnter);

    JSValue onMouseExit = JS_GetPropertyStr(ctx, props, "onMouseExit");
    if (JS_IsFunction(ctx, onMouseExit)) {
        auto* engine = static_cast<QuickJSEngine*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
        engine->registerMouseExitHandler(component, ctx, onMouseExit);
    }
    JS_FreeValue(ctx, onMouseExit);

}

static void buildComponentTree(JSContext* ctx, JSValueConst node, IRenderer* renderer, void* component) {
    JSValue props = JS_GetPropertyStr(ctx, node, "props");
    applyPropsToFrame(ctx, props, renderer, component);
    JSValue children = JS_GetPropertyStr(ctx, props, "children");

    if (JS_IsArray(children)) {
        uint32_t length = 0;
        JSValue lengthVal = JS_GetPropertyStr(ctx, children, "length");
        JS_ToUint32(ctx, &length, lengthVal);
        JS_FreeValue(ctx, lengthVal);

        for (uint32_t i = 0; i < length; ++i) {
            JSValue child = JS_GetPropertyUint32(ctx, children, i);

            void* childComponent = renderer->createComponent(component);

            buildComponentTree(ctx, child, renderer, childComponent);
            JS_FreeValue(ctx, child);
        }
    }

    JS_FreeValue(ctx, children);
    JS_FreeValue(ctx, props);
}


static void callApp(JSContext* ctx, IRenderer* renderer) {
    auto* engine = static_cast<QuickJSEngine*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
    
    if (JS_IsUndefined(engine->appFn_)) {
        return;
    }

    auto* root = renderer->getRootComponent();

    JSValue result = JS_Call(ctx, engine->appFn_, JS_UNDEFINED, 0, nullptr);
    if (JS_IsException(result)) {
        js_std_dump_error(ctx);
        return;
    }

    buildComponentTree(ctx, result, renderer, root);
    JS_FreeValue(ctx, result);
}

QuickJSEngine::QuickJSEngine(IParameterProvider &parameterStore)
: parameterStore_(parameterStore)
{
    
}

void QuickJSEngine::load(const std::string &entryFile, IRenderer *renderer)
{
    if (ctx_) {
        if (renderer_)
            renderer_->clear();

        for (auto& fn : drawCallbacks_)
            JS_FreeValue(ctx_, fn);
        drawCallbacks_.clear();

        for (auto& [component, fn] : mouseDownHandlers_)
            JS_FreeValue(ctx_, fn);
        mouseDownHandlers_.clear();

        for (auto& [component, fn] : mouseUpHandlers_)
            JS_FreeValue(ctx_, fn);
        mouseUpHandlers_.clear();

        for (auto& [component, fn] : mouseDragHandlers_)
            JS_FreeValue(ctx_, fn);
        mouseDragHandlers_.clear();

        for (auto& [component, fn] : mouseEnterHandlers_)
            JS_FreeValue(ctx_, fn);
        mouseEnterHandlers_.clear();

        for (auto& [component, fn] : mouseExitHandlers_)
            JS_FreeValue(ctx_, fn);
        mouseExitHandlers_.clear();

        if (!JS_IsUndefined(appFn_)) {
            JS_FreeValue(ctx_, appFn_);
            appFn_ = JS_UNDEFINED;
        }

        JS_FreeContext(ctx_);
        ctx_ = nullptr;
    }

    if (rt_) {
        JS_FreeRuntime(rt_);
        rt_ = nullptr;
    }

    renderer_ = renderer;
    rt_ = JS_NewRuntime();
    JS_SetRuntimeOpaque(rt_, this);
    js_std_init_handlers(rt_);
    JS_SetModuleLoaderFunc2(rt_, NULL, js_module_loader, js_module_check_attributes, NULL);

    ctx_ = JS_NewContext(rt_);

    js_init_module_singularity(ctx_, "singularity");

    size_t buf_len;
    installConsole();

#if !defined NDEBUG
    auto buf = js_load_file(ctx_, &buf_len, entryFile.c_str());

    if (!buf) {
        std::cout << "Failed loading js file\n";
        return;
    }

    auto module_val = JS_Eval(ctx_, (const char*)buf, buf_len, entryFile.c_str(), JS_EVAL_TYPE_MODULE);

    if (JS_IsException(module_val)) {
        js_std_dump_error(ctx_);
    } else {
        callApp(ctx_, renderer_); // <-- same as before
    }

    JS_FreeValue(ctx_, module_val);
    js_free(ctx_, buf);
#else
    js_std_eval_binary(ctx_, QSJC_SYMBOL, QSJC_SYMBOL_SIZE, 0);
    callApp(ctx_, renderer_); // <-- same as before
#endif
}

void QuickJSEngine::registerMouseDownHandler(void* component, JSContext* ctx, JSValue fn)
{
    auto it = mouseDownHandlers_.find(component);
    if (it != mouseDownHandlers_.end()) {
        JS_FreeValue(ctx, it->second);
    }

    mouseDownHandlers_[component] = JS_DupValue(ctx, fn);
}

void QuickJSEngine::registerMouseUpHandler(void* component, JSContext* ctx, JSValue fn)
{
    auto it = mouseUpHandlers_.find(component);
    if (it != mouseUpHandlers_.end()) {
        JS_FreeValue(ctx, it->second);
    }

    mouseUpHandlers_[component] = JS_DupValue(ctx, fn);
}

void QuickJSEngine::registerMouseDragHandler(void* component, JSContext* ctx, JSValue fn)
{
    auto it = mouseDragHandlers_.find(component);
    if (it != mouseDragHandlers_.end()) {
        JS_FreeValue(ctx, it->second);
    }

    mouseDragHandlers_[component] = JS_DupValue(ctx, fn);
}

void QuickJSEngine::registerMouseEnterHandler(void* component, JSContext* ctx, JSValue fn)
{
    auto it = mouseEnterHandlers_.find(component);
    if (it != mouseEnterHandlers_.end())
        JS_FreeValue(ctx, it->second);
    mouseEnterHandlers_[component] = JS_DupValue(ctx, fn);
}

void QuickJSEngine::registerMouseExitHandler(void* component, JSContext* ctx, JSValue fn)
{
    auto it = mouseExitHandlers_.find(component);
    if (it != mouseExitHandlers_.end())
        JS_FreeValue(ctx, it->second);
    mouseExitHandlers_[component] = JS_DupValue(ctx, fn);
}

void QuickJSEngine::onMouseDown(void *component, float x, float y)
{
    auto it = mouseDownHandlers_.find(component);
    if (it == mouseDownHandlers_.end())
        return;

    JSValue eventObj = JS_NewObject(ctx_);
    JS_SetPropertyStr(ctx_, eventObj, "x", JS_NewFloat64(ctx_, x));
    JS_SetPropertyStr(ctx_, eventObj, "y", JS_NewFloat64(ctx_, y));

    JSValue argv[1] = { eventObj };
    JSValue result = JS_Call(ctx_, it->second, JS_UNDEFINED, 1, argv);

    if (JS_IsException(result))
        js_std_dump_error(ctx_);

    JS_FreeValue(ctx_, result);
    JS_FreeValue(ctx_, eventObj);

    renderer_->redraw(component);
}

void QuickJSEngine::onMouseUp(void* component, float x, float y)
{
    auto it = mouseUpHandlers_.find(component);
    if (it == mouseUpHandlers_.end())
        return;

    JSValue eventObj = JS_NewObject(ctx_);
    JS_SetPropertyStr(ctx_, eventObj, "x", JS_NewFloat64(ctx_, x));
    JS_SetPropertyStr(ctx_, eventObj, "y", JS_NewFloat64(ctx_, y));

    JSValue argv[1] = { eventObj };
    JSValue result = JS_Call(ctx_, it->second, JS_UNDEFINED, 1, argv);

    if (JS_IsException(result))
        js_std_dump_error(ctx_);

    JS_FreeValue(ctx_, result);
    JS_FreeValue(ctx_, eventObj);

    renderer_->redraw(component);
}

void QuickJSEngine::onMouseDrag(void* component, float x, float y)
{
    auto it = mouseDragHandlers_.find(component);
    if (it == mouseDragHandlers_.end())
        return;

    JSValue eventObj = JS_NewObject(ctx_);
    JS_SetPropertyStr(ctx_, eventObj, "x", JS_NewFloat64(ctx_, x));
    JS_SetPropertyStr(ctx_, eventObj, "y", JS_NewFloat64(ctx_, y));

    JSValue argv[1] = { eventObj };
    JSValue result = JS_Call(ctx_, it->second, JS_UNDEFINED, 1, argv);

    if (JS_IsException(result))
        js_std_dump_error(ctx_);

    JS_FreeValue(ctx_, result);
    JS_FreeValue(ctx_, eventObj);

    renderer_->redraw(component);
}

void QuickJSEngine::onMouseEnter(void* component)
{
    auto it = mouseEnterHandlers_.find(component);
    if (it == mouseEnterHandlers_.end())
        return;

    JSValue result = JS_Call(ctx_, it->second, JS_UNDEFINED, 0, nullptr);
    if (JS_IsException(result))
        js_std_dump_error(ctx_);
    JS_FreeValue(ctx_, result);

    renderer_->redraw(component);
}

void QuickJSEngine::onMouseExit(void* component)
{
    auto it = mouseExitHandlers_.find(component);
    if (it == mouseExitHandlers_.end())
        return;

    JSValue result = JS_Call(ctx_, it->second, JS_UNDEFINED, 0, nullptr);
    if (JS_IsException(result))
        js_std_dump_error(ctx_);
    JS_FreeValue(ctx_, result);

    renderer_->redraw(component);
}

JSValue QuickJSEngine::setParameter(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    int32_t parameterId;
    double parameterValue;

    JS_ToInt32(ctx, &parameterId, argv[0]);
    JS_ToFloat64(ctx, &parameterValue, argv[1]);

    parameterStore_.setParameter(parameterId, parameterValue);

    // Redraw root so all components bound to this parameter update
    renderer_->redrawAll();

    return JS_UNDEFINED;
}

void QuickJSEngine::log(const std::string& msg)
{
    if (logger_)
        logger_(msg);
}

JSValue QuickJSEngine::getParameter(JSContext* ctx, JSValueConst this_val,
                                    int argc, JSValueConst* argv)
{
    if (argc < 1)
        return JS_ThrowTypeError(ctx, "getParameter(id) expects parameter id");

    int32_t parameterId = 0;
    if (JS_ToInt32(ctx, &parameterId, argv[0]) < 0)
        return JS_EXCEPTION;

    double value = parameterStore_.getParameter(parameterId);
    return JS_NewFloat64(ctx, value);
}