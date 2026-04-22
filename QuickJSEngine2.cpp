#include "QuickJSEngine2.h"
#include "IJSEngine.h"
#include <iostream>
#include "QuickJSEngineCanvasAPI.h"

std::unique_ptr<IJSEngine> IJSEngine::createJSEngine()
{
    return std::make_unique<QuickJSEngine>();
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

static const JSCFunctionListEntry singularity_funcs[] = {
    JS_CFUNC_DEF("Component", 1, js_Component),
    JS_CFUNC_DEF("mount", 1, js_mount),
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

    JSValue draw = JS_GetPropertyStr(ctx, props, "draw");

    if (JS_IsFunction(ctx, draw)) {
        auto* engine = static_cast<QuickJSEngine*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
        JSValue drawFn = JS_DupValue(ctx, draw);
        engine->drawCallbacks_.push_back(drawFn);
        renderer->setDrawCallback(component, [ctx, drawFn, renderer](void* canvas) mutable {
            DrawContextData drawData;
            drawData.renderer = renderer;
            drawData.canvas = canvas;

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

            // CANVAS API PROPERTIES (setters)
            JS_DefinePropertyGetSet(ctx, jsCtx, JS_NewAtom(ctx, "fillStyle"),    JS_UNDEFINED, JS_NewCFunction(ctx, js_fillStyle,    "fillStyle",    1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
            JS_DefinePropertyGetSet(ctx, jsCtx, JS_NewAtom(ctx, "strokeStyle"),  JS_UNDEFINED, JS_NewCFunction(ctx, js_strokeStyle,  "strokeStyle",  1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
            JS_DefinePropertyGetSet(ctx, jsCtx, JS_NewAtom(ctx, "lineWidth"),    JS_UNDEFINED, JS_NewCFunction(ctx, js_lineWidth,    "lineWidth",    1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
            JS_DefinePropertyGetSet(ctx, jsCtx, JS_NewAtom(ctx, "font"),         JS_UNDEFINED, JS_NewCFunction(ctx, js_font,         "font",         1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
            JS_DefinePropertyGetSet(ctx, jsCtx, JS_NewAtom(ctx, "globalAlpha"),  JS_UNDEFINED, JS_NewCFunction(ctx, js_globalAlpha,  "globalAlpha",  1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
            JS_DefinePropertyGetSet(ctx, jsCtx, JS_NewAtom(ctx, "textAlign"),    JS_UNDEFINED, JS_NewCFunction(ctx, js_textAlign,    "textAlign",    1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
            JS_DefinePropertyGetSet(ctx, jsCtx, JS_NewAtom(ctx, "textBaseline"), JS_UNDEFINED, JS_NewCFunction(ctx, js_textBaseline, "textBaseline", 1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);

            JSValue argv[1] = { jsCtx };
            JSValue result = JS_Call(ctx, drawFn, JS_UNDEFINED, 1, argv);

            if (JS_IsException(result))
                js_std_dump_error(ctx);

            JS_FreeValue(ctx, result);
            JS_FreeValue(ctx, jsCtx);

            JS_SetContextOpaque(ctx, previousOpaque);
        });
    }

    JS_FreeValue(ctx, draw);
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

void QuickJSEngine::load(const std::string &entryFile, IRenderer *renderer)
{
    if (ctx_) {
        if (renderer_)
            renderer_->clear();

        for (auto& fn : drawCallbacks_)
            JS_FreeValue(ctx_, fn);
        drawCallbacks_.clear();

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
    js_std_add_helpers(ctx_, 0, nullptr);

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
}
