#include "QuickJSEngine2.h"
#include "IJSEngine.h"
#include "platform/IWindow.h"
#include <iostream>
#include <filesystem>
#include <vector>
#include "QuickJSEngineCanvasAPI.h"

#ifdef NDEBUG
#include "generated_loader.h"
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

static JSValue js_openFileDialog(JSContext* ctx, JSValueConst this_val,
                                  int argc, JSValueConst* argv)
{
    auto* engine = static_cast<QuickJSEngine*>(
        JS_GetRuntimeOpaque(JS_GetRuntime(ctx))
    );

    if (!engine->window())
        return JS_ThrowInternalError(ctx, "No window attached");

    std::string title = "Open File";
    if (argc > 0) {
        const char* t = JS_ToCString(ctx, argv[0]);
        if (t) { title = t; JS_FreeCString(ctx, t); }
    }

    JSValue callback = JS_UNDEFINED;
    if (argc > 1 && JS_IsFunction(ctx, argv[1])) {
        callback = JS_DupValue(ctx, argv[1]);
    }

    engine->window()->openFileDialog(title, [engine, ctx, callback](const std::string& path) mutable {
        if (!JS_IsUndefined(callback)) {
            JSValue arg = JS_NewString(ctx, path.c_str());
            JSValue ret = JS_Call(ctx, callback, JS_UNDEFINED, 1, &arg);
            JS_FreeValue(ctx, ret);
            JS_FreeValue(ctx, arg);
            JS_FreeValue(ctx, callback);
            callback = JS_UNDEFINED;
        }
    });

    return JS_UNDEFINED;
}

static const JSCFunctionListEntry singularity_funcs[] = {
    JS_CFUNC_DEF("Component", 1, js_Component),
    JS_CFUNC_DEF("mount", 1, js_mount),
    JS_CFUNC_DEF("getParameter", 1, js_getParameter),
    JS_CFUNC_DEF("setParameter", 2, js_setParameter),
    JS_CFUNC_DEF("openFileDialog", 2, js_openFileDialog),
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

static float getFloatProp(JSContext* ctx, JSValueConst props, const char* name, float fallback = 0.f) {
    JSValue val = JS_GetPropertyStr(ctx, props, name);
    double out = fallback;
    if (!JS_IsUndefined(val) && !JS_IsNull(val))
        JS_ToFloat64(ctx, &out, val);
    JS_FreeValue(ctx, val);
    return static_cast<float>(out);
}

static void buildComponentTree(JSContext* ctx, JSValueConst node, IRenderer* renderer, float offsetX = 0.f, float offsetY = 0.f, bool isRoot = false, bool ignoreOwnPosition = false) {
    JSValue props = JS_GetPropertyStr(ctx, node, "props");

    // Resize renderer if root component declares width/height
    int w_int = getIntProp(ctx, props, "width", -1);
    int h_int = getIntProp(ctx, props, "height", -1);
    if (w_int != -1 && h_int != -1 && isRoot)
        renderer->resize(w_int, h_int);

    float localX = ignoreOwnPosition ? 0.f : getFloatProp(ctx, props, "x", 0.f);
    float localY = ignoreOwnPosition ? 0.f : getFloatProp(ctx, props, "y", 0.f);
    float absX   = offsetX + localX;
    float absY   = offsetY + localY;
    float w      = getFloatProp(ctx, props, "width",  0.f);
    float h      = getFloatProp(ctx, props, "height", 0.f);

    // margin: uniform space outside the component — shifts position inward, shrinks rendered size.
    // Works whether positioned absolutely or placed by a flex parent.
    float margin = getFloatProp(ctx, props, "margin", 0.f);
    absX += margin;
    absY += margin;
    if (w > 0.f) w -= 2.f * margin;
    if (h > 0.f) h -= 2.f * margin;

    // backgroundColor
    std::string bgColor;
    {
        JSValue bgVal = JS_GetPropertyStr(ctx, props, "backgroundColor");
        if (JS_IsString(bgVal)) {
            const char* s = JS_ToCString(ctx, bgVal);
            if (s) {
                bgColor = s;
                if (isRoot) {
                    auto* engine = static_cast<QuickJSEngine*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
                    engine->backgroundColor_ = s;
                }
                JS_FreeCString(ctx, s);
            }
        }
        JS_FreeValue(ctx, bgVal);
    }

    float       borderRadius = getFloatProp(ctx, props, "borderRadius", 0.f);
    std::string borderColor;
    float       borderWidth  = 1.f;
    {
        JSValue bVal = JS_GetPropertyStr(ctx, props, "border");
        if (JS_IsObject(bVal)) {
            JSValue cVal = JS_GetPropertyStr(ctx, bVal, "color");
            if (JS_IsString(cVal)) {
                const char* s = JS_ToCString(ctx, cVal);
                if (s) { borderColor = s; JS_FreeCString(ctx, s); }
            }
            JS_FreeValue(ctx, cVal);
            borderWidth = getFloatProp(ctx, bVal, "width", 1.f);
        }
        JS_FreeValue(ctx, bVal);
    }

    // Clip + background: if this component has bounds, push a ClipBegin entry
    // so children are clipped to this component's rect
    bool hasClip = (w > 0.f && h > 0.f);
    if (hasClip) {
        auto* engine = static_cast<QuickJSEngine*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
        QuickJSEngine::DrawEntry clipBegin;
        clipBegin.type         = QuickJSEngine::DrawEntryType::ClipBegin;
        clipBegin.absX         = absX;
        clipBegin.absY         = absY;
        clipBegin.width        = w;
        clipBegin.height       = h;
        clipBegin.bgColor      = bgColor;
        clipBegin.borderRadius = borderRadius;
        clipBegin.borderColor  = borderColor;
        clipBegin.borderWidth  = borderWidth;
        engine->drawEntries_.push_back(std::move(clipBegin));
    }

    // Collect hitbox for mouse routing (only if the component has bounds)
    if (w > 0.f && h > 0.f) {
        auto* engine = static_cast<QuickJSEngine*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
        QuickJSEngine::Hitbox hb;
        hb.x = absX; hb.y = absY; hb.w = w; hb.h = h;

        JSValue fn;
        fn = JS_GetPropertyStr(ctx, props, "onMouseDown");
        hb.onMouseDown = JS_IsFunction(ctx, fn) ? fn : (JS_FreeValue(ctx, fn), JS_UNDEFINED);

        fn = JS_GetPropertyStr(ctx, props, "onMouseUp");
        hb.onMouseUp = JS_IsFunction(ctx, fn) ? fn : (JS_FreeValue(ctx, fn), JS_UNDEFINED);

        fn = JS_GetPropertyStr(ctx, props, "onMouseDrag");
        hb.onMouseDrag = JS_IsFunction(ctx, fn) ? fn : (JS_FreeValue(ctx, fn), JS_UNDEFINED);

        fn = JS_GetPropertyStr(ctx, props, "onMouseWheel");
        hb.onMouseWheel = JS_IsFunction(ctx, fn) ? fn : (JS_FreeValue(ctx, fn), JS_UNDEFINED);

        engine->hitboxes_.push_back(std::move(hb));
    }

    // Collect draw entry (absolute position + draw fn so draw() can translate)
    {
        auto* engine = static_cast<QuickJSEngine*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
        JSValue drawFn = JS_GetPropertyStr(ctx, props, "draw");
        if (JS_IsFunction(ctx, drawFn)) {
            QuickJSEngine::DrawEntry entry;
            entry.absX = absX;
            entry.absY = absY;
            entry.fn   = drawFn;
            engine->drawEntries_.push_back(std::move(entry));
        } else {
            JS_FreeValue(ctx, drawFn);
        }
    }

    // Animate opt-in: any component with animate:true drives continuous per-frame redraws
    {
        JSValue animVal = JS_GetPropertyStr(ctx, props, "animate");
        if (JS_ToBool(ctx, animVal))
            static_cast<QuickJSEngine*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)))->hasAnimations_ = true;
        JS_FreeValue(ctx, animVal);
    }

    // Read layout properties
    JSValue flexDirVal = JS_GetPropertyStr(ctx, props, "flexDirection");
    std::string flexDir;
    if (JS_IsString(flexDirVal)) {
        const char* s = JS_ToCString(ctx, flexDirVal);
        if (s) { flexDir = s; JS_FreeCString(ctx, s); }
    }
    JS_FreeValue(ctx, flexDirVal);

    float gap     = getFloatProp(ctx, props, "gap", 0.f);
    float padding = getFloatProp(ctx, props, "padding", 0.f);

    JSValue children = JS_GetPropertyStr(ctx, props, "children");
    if (JS_IsArray(children)) {
        uint32_t length = 0;
        JSValue lengthVal = JS_GetPropertyStr(ctx, children, "length");
        JS_ToUint32(ctx, &length, lengthVal);
        JS_FreeValue(ctx, lengthVal);

        if (!flexDir.empty()) {
            // Flex layout: position children along main axis
            bool isRow = (flexDir == "row");
            float containerSize = isRow ? w : h;

            // Read justifyContent
            JSValue jcVal = JS_GetPropertyStr(ctx, props, "justifyContent");
            std::string justifyContent = "flex-start";
            if (JS_IsString(jcVal)) {
                const char* s = JS_ToCString(ctx, jcVal);
                if (s) { justifyContent = s; JS_FreeCString(ctx, s); }
            }
            JS_FreeValue(ctx, jcVal);

            // Read alignItems
            JSValue aiVal = JS_GetPropertyStr(ctx, props, "alignItems");
            std::string alignItems = "flex-start";
            if (JS_IsString(aiVal)) {
                const char* s = JS_ToCString(ctx, aiVal);
                if (s) { alignItems = s; JS_FreeCString(ctx, s); }
            }
            JS_FreeValue(ctx, aiVal);

            float crossContainerSize = isRow ? h : w;
            float crossAvailable = crossContainerSize - 2.f * padding;

            // First pass: collect child main-axis and cross-axis sizes
            std::vector<float> childSizes(length);
            std::vector<float> childCrossSizes(length);
            float totalChildSize = 0.f;
            for (uint32_t i = 0; i < length; ++i) {
                JSValue child = JS_GetPropertyUint32(ctx, children, i);
                JSValue childProps = JS_GetPropertyStr(ctx, child, "props");
                float sz = isRow ? getFloatProp(ctx, childProps, "width",  0.f)
                                 : getFloatProp(ctx, childProps, "height", 0.f);
                float crossSz = isRow ? getFloatProp(ctx, childProps, "height", 0.f)
                                      : getFloatProp(ctx, childProps, "width",  0.f);
                float childMargin = getFloatProp(ctx, childProps, "margin", 0.f);
                // margin expands the space a child occupies in the flex layout
                childSizes[i]     = sz     + 2.f * childMargin;
                childCrossSizes[i] = crossSz + 2.f * childMargin;
                totalChildSize += sz;
                JS_FreeValue(ctx, childProps);
                JS_FreeValue(ctx, child);
            }

            // Distributing modes ignore gap — spacing is computed entirely from available space.
            // gap only applies for flex-start / center / flex-end.
            bool isDistributing = (justifyContent == "space-between" ||
                                   justifyContent == "space-around"  ||
                                   justifyContent == "space-evenly");

            float totalContentSize = totalChildSize;
            if (!isDistributing && length > 1) totalContentSize += gap * (length - 1);

            float availableSpace = containerSize - 2.f * padding;
            float remainingSpace = availableSpace - totalContentSize;

            // Compute starting cursor and per-item spacing based on justifyContent
            float cursor = padding;
            float itemSpacing = gap;

            if (justifyContent == "center") {
                cursor = padding + remainingSpace / 2.f;
            } else if (justifyContent == "flex-end") {
                cursor = padding + remainingSpace;
            } else if (justifyContent == "space-between") {
                float freeSpace = availableSpace - totalChildSize;
                cursor = padding;
                itemSpacing = length > 1 ? freeSpace / (float)(length - 1) : 0.f;
            } else if (justifyContent == "space-around") {
                float freeSpace = availableSpace - totalChildSize;
                float space = length > 0 ? freeSpace / (float)length : 0.f;
                cursor = padding + space / 2.f;
                itemSpacing = space;
            } else if (justifyContent == "space-evenly") {
                float freeSpace = availableSpace - totalChildSize;
                float space = length > 0 ? freeSpace / (float)(length + 1) : 0.f;
                cursor = padding + space;
                itemSpacing = space;
            }
            // else "flex-start": cursor = padding, itemSpacing = gap

            // Second pass: place children
            for (uint32_t i = 0; i < length; ++i) {
                JSValue child = JS_GetPropertyUint32(ctx, children, i);

                // Compute cross-axis offset based on alignItems
                float crossOffset = padding; // flex-start default
                if (alignItems == "center") {
                    crossOffset = padding + (crossAvailable - childCrossSizes[i]) / 2.f;
                } else if (alignItems == "flex-end") {
                    crossOffset = padding + crossAvailable - childCrossSizes[i];
                }

                float childOffsetX = isRow ? cursor : crossOffset;
                float childOffsetY = isRow ? crossOffset : cursor;

                buildComponentTree(ctx, child, renderer, absX + childOffsetX, absY + childOffsetY, false, true);

                cursor += childSizes[i] + itemSpacing;

                JS_FreeValue(ctx, child);
            }
        } else {
            // Absolute positioning (default behavior)
            for (uint32_t i = 0; i < length; ++i) {
                JSValue child = JS_GetPropertyUint32(ctx, children, i);
                buildComponentTree(ctx, child, renderer, absX, absY);
                JS_FreeValue(ctx, child);
            }
        }
    }

    JS_FreeValue(ctx, children);

    // Close the clip region if we opened one
    if (hasClip) {
        auto* engine = static_cast<QuickJSEngine*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
        QuickJSEngine::DrawEntry clipEnd;
        clipEnd.type = QuickJSEngine::DrawEntryType::ClipEnd;
        engine->drawEntries_.push_back(std::move(clipEnd));
    }

    JS_FreeValue(ctx, props);
}


static void callApp(JSContext* ctx, IRenderer* renderer) {
    auto* engine = static_cast<QuickJSEngine*>(JS_GetRuntimeOpaque(JS_GetRuntime(ctx)));
    
    if (JS_IsUndefined(engine->appFn_)) {
        return;
    }

    JSValue result = JS_Call(ctx, engine->appFn_, JS_UNDEFINED, 0, nullptr);
    if (JS_IsException(result)) {
        js_std_dump_error(ctx);
        return;
    }

    buildComponentTree(ctx, result, renderer, 0.f, 0.f, true);
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
            // renderer_->clear();

        for (auto& e : drawEntries_) {
            if (!JS_IsUndefined(e.fn))
                JS_FreeValue(ctx_, e.fn);
        }
        drawEntries_.clear();
        backgroundColor_.clear();
        hasAnimations_ = false;

        for (auto& hb : hitboxes_) {
            JS_FreeValue(ctx_, hb.onMouseDown);
            JS_FreeValue(ctx_, hb.onMouseUp);
            JS_FreeValue(ctx_, hb.onMouseDrag);
            JS_FreeValue(ctx_, hb.onMouseWheel);
        }
        hitboxes_.clear();

        JS_FreeValue(ctx_, activeDragFn_);    activeDragFn_    = JS_UNDEFINED;
        JS_FreeValue(ctx_, activeMouseUpFn_); activeMouseUpFn_ = JS_UNDEFINED;
        dragging_ = false;

        if (!JS_IsUndefined(appFn_)) {
            JS_FreeValue(ctx_, appFn_);
            appFn_ = JS_UNDEFINED;
        }

        if (!JS_IsUndefined(jsCanvasCtx_)) {
            JS_FreeValue(ctx_, jsCanvasCtx_);
            jsCanvasCtx_ = JS_UNDEFINED;
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

    // Custom normalizer: maps "singularity/<widget>.js" → SINGULARITY_WIDGETS_DIR/<widget>.js
    // so users can write  import { Knob } from "singularity/knob.js"  regardless of where
    // their plugin lives.  Any other path falls through to the standard relative-path resolver.
    auto normalizer = [](JSContext* ctx,
                        const char* base_name,
                        const char* name,
                        void*) -> char* {
        std::string_view mod(name);

        // Built-in/root singularity module
        if (mod == "singularity") {
            return js_strdup(ctx, name);
        }

        // Widget imports: singularity/knob.js, singularity/button.js, etc.
        if (mod.starts_with("singularity/") && mod.size() > 12) {
            std::string widget(mod);

            if (!widget.ends_with(".js"))
                widget += ".js";

    #ifdef NDEBUG
            // Release: keep virtual name so it matches qjsc embedded bytecode
            return js_strdup(ctx, widget.c_str());
    #else
            // Debug: load widget source from filesystem
            std::string path =
                std::string(SINGULARITY_WIDGETS_DIR) + "/" +
                std::string(widget.substr(12));

            return js_strdup(ctx, path.c_str());
    #endif
        }

        // Relative imports: ./App.js, ../foo.js, etc.
        if (mod.starts_with("./") || mod.starts_with("../")) {
            namespace fs = std::filesystem;

            fs::path base = base_name ? fs::path(base_name).parent_path() : fs::path(".");
            fs::path resolved = (base / std::string(mod)).lexically_normal();

            return js_strdup(ctx, resolved.generic_string().c_str());
        }

        // Everything else unchanged
        return js_strdup(ctx, name);
    };

    JS_SetModuleLoaderFunc2(rt_, normalizer, js_module_loader, js_module_check_attributes, NULL);

    ctx_ = JS_NewContext(rt_);

    js_init_module_singularity(ctx_, "singularity");

    size_t buf_len;
    installConsole();

#ifndef NDEBUG
    // If the entry point is App.js, synthesize the mount wrapper in-memory so
    // plugin authors don't need a boilerplate main.js in their source tree.
    std::filesystem::path entryPath(entryFile);
    if (entryPath.filename() == "App.js") {
        std::string synthetic =
            "import App from \"" + entryFile + "\";\n"
            "import { mount } from \"singularity\";\n"
            "mount(App);\n";
        auto module_val = JS_Eval(ctx_, synthetic.c_str(), synthetic.size(),
                                  "<main>", JS_EVAL_TYPE_MODULE);
        if (JS_IsException(module_val)) {
            js_std_dump_error(ctx_);
        } else {
            callApp(ctx_, renderer_);
        }
        JS_FreeValue(ctx_, module_val);
    } else {
        auto buf = js_load_file(ctx_, &buf_len, entryFile.c_str());

        if (!buf) {
            std::cout << "Failed loading js file\n";
            return;
        }

        auto module_val = JS_Eval(ctx_, (const char*)buf, buf_len, entryFile.c_str(), JS_EVAL_TYPE_MODULE);

        if (JS_IsException(module_val)) {
            js_std_dump_error(ctx_);
        } else {
            callApp(ctx_, renderer_);
        }

        JS_FreeValue(ctx_, module_val);
        js_free(ctx_, buf);
    }
#else
    qjsc_load_modules(ctx_);
    callApp(ctx_, renderer_);
#endif

    // Create the persistent canvas context object once — reused every frame
    createCanvasContext();
}

void QuickJSEngine::createCanvasContext()
{
    if (!ctx_) return;

    jsCanvasCtx_ = JS_NewObject(ctx_);

    // Methods
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "fillRect",           JS_NewCFunction(ctx_, js_fillRect,           "fillRect",           4));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "strokeRect",         JS_NewCFunction(ctx_, js_strokeRect,         "strokeRect",         4));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "clearRect",          JS_NewCFunction(ctx_, js_clearRect,          "clearRect",          4));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "beginPath",          JS_NewCFunction(ctx_, js_beginPath,          "beginPath",          0));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "closePath",          JS_NewCFunction(ctx_, js_closePath,          "closePath",          0));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "moveTo",             JS_NewCFunction(ctx_, js_moveTo,             "moveTo",             2));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "lineTo",             JS_NewCFunction(ctx_, js_lineTo,             "lineTo",             2));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "arc",                JS_NewCFunction(ctx_, js_arc,                "arc",                5));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "arcTo",              JS_NewCFunction(ctx_, js_arcTo,              "arcTo",              5));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "quadraticCurveTo",   JS_NewCFunction(ctx_, js_quadraticCurveTo,   "quadraticCurveTo",   4));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "bezierCurveTo",      JS_NewCFunction(ctx_, js_bezierCurveTo,      "bezierCurveTo",      6));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "ellipse",            JS_NewCFunction(ctx_, js_ellipse,            "ellipse",            7));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "rect",               JS_NewCFunction(ctx_, js_rect,               "rect",               4));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "roundRect",          JS_NewCFunction(ctx_, js_roundRect,          "roundRect",          5));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "fill",               JS_NewCFunction(ctx_, js_fill,               "fill",               0));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "stroke",             JS_NewCFunction(ctx_, js_stroke,             "stroke",             0));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "fillText",           JS_NewCFunction(ctx_, js_fillText,           "fillText",           3));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "strokeText",         JS_NewCFunction(ctx_, js_strokeText,         "strokeText",         3));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "measureText",        JS_NewCFunction(ctx_, js_measureText,        "measureText",        1));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "save",               JS_NewCFunction(ctx_, js_save,               "save",               0));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "restore",            JS_NewCFunction(ctx_, js_restore,            "restore",            0));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "translate",          JS_NewCFunction(ctx_, js_translate,          "translate",          2));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "rotate",             JS_NewCFunction(ctx_, js_rotate,             "rotate",             1));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "scale",              JS_NewCFunction(ctx_, js_scale,              "scale",              2));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "resetTransform",     JS_NewCFunction(ctx_, js_resetTransform,     "resetTransform",     0));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "createLinearGradient", JS_NewCFunction(ctx_, js_createLinearGradient, "createLinearGradient", 4));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "createRadialGradient", JS_NewCFunction(ctx_, js_createRadialGradient, "createRadialGradient", 6));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "drawImage",          JS_NewCFunction(ctx_, js_drawImage,          "drawImage",          5));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "drawShader",         JS_NewCFunction(ctx_, js_drawShader,         "drawShader",         6));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "drawShaderWithImage",JS_NewCFunction(ctx_, js_drawShaderWithImage,"drawShaderWithImage", 7));
    JS_SetPropertyStr(ctx_, jsCanvasCtx_, "time",               JS_NewCFunction(ctx_, js_time,               "time",               0));

    // Properties (write-only setters)
    JS_DefinePropertyGetSet(ctx_, jsCanvasCtx_, JS_NewAtom(ctx_, "fillStyle"),    JS_UNDEFINED, JS_NewCFunction(ctx_, js_fillStyle,    "fillStyle",    1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_DefinePropertyGetSet(ctx_, jsCanvasCtx_, JS_NewAtom(ctx_, "strokeStyle"),  JS_UNDEFINED, JS_NewCFunction(ctx_, js_strokeStyle,  "strokeStyle",  1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_DefinePropertyGetSet(ctx_, jsCanvasCtx_, JS_NewAtom(ctx_, "lineWidth"),    JS_UNDEFINED, JS_NewCFunction(ctx_, js_lineWidth,    "lineWidth",    1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_DefinePropertyGetSet(ctx_, jsCanvasCtx_, JS_NewAtom(ctx_, "lineCap"),      JS_UNDEFINED, JS_NewCFunction(ctx_, js_lineCap,      "lineCap",      1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_DefinePropertyGetSet(ctx_, jsCanvasCtx_, JS_NewAtom(ctx_, "font"),         JS_UNDEFINED, JS_NewCFunction(ctx_, js_font,         "font",         1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_DefinePropertyGetSet(ctx_, jsCanvasCtx_, JS_NewAtom(ctx_, "globalAlpha"),  JS_UNDEFINED, JS_NewCFunction(ctx_, js_globalAlpha,  "globalAlpha",  1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_DefinePropertyGetSet(ctx_, jsCanvasCtx_, JS_NewAtom(ctx_, "textAlign"),    JS_UNDEFINED, JS_NewCFunction(ctx_, js_textAlign,    "textAlign",    1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_DefinePropertyGetSet(ctx_, jsCanvasCtx_, JS_NewAtom(ctx_, "textBaseline"), JS_UNDEFINED, JS_NewCFunction(ctx_, js_textBaseline, "textBaseline", 1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_DefinePropertyGetSet(ctx_, jsCanvasCtx_, JS_NewAtom(ctx_, "shadowColor"),  JS_UNDEFINED, JS_NewCFunction(ctx_, js_shadowColor,  "shadowColor",  1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_DefinePropertyGetSet(ctx_, jsCanvasCtx_, JS_NewAtom(ctx_, "shadowBlur"),   JS_UNDEFINED, JS_NewCFunction(ctx_, js_shadowBlur,   "shadowBlur",   1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_DefinePropertyGetSet(ctx_, jsCanvasCtx_, JS_NewAtom(ctx_, "shadowOffsetX"),JS_UNDEFINED, JS_NewCFunction(ctx_, js_shadowOffsetX,"shadowOffsetX",1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_DefinePropertyGetSet(ctx_, jsCanvasCtx_, JS_NewAtom(ctx_, "shadowOffsetY"),JS_UNDEFINED, JS_NewCFunction(ctx_, js_shadowOffsetY,"shadowOffsetY",1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
    JS_DefinePropertyGetSet(ctx_, jsCanvasCtx_, JS_NewAtom(ctx_, "bloom"),         JS_UNDEFINED, JS_NewCFunction(ctx_, js_bloom,         "bloom",         1), JS_PROP_CONFIGURABLE | JS_PROP_ENUMERABLE);
}

void QuickJSEngine::draw()
{
    if (!ctx_ || !renderer_ || JS_IsUndefined(jsCanvasCtx_)) return;

    DrawContextData drawData;
    drawData.renderer  = renderer_;
    drawData.canvas    = nullptr;
    drawData.component = nullptr;

    void* previousOpaque = JS_GetContextOpaque(ctx_);
    JS_SetContextOpaque(ctx_, &drawData);

    // Reset per-frame renderer state
    renderer_->setBloom(0.0f);

    // Paint background color
    if (!backgroundColor_.empty()) {
        renderer_->setFillStyle(backgroundColor_);
        renderer_->fillRect(0, 0, (float)renderer_->width(), (float)renderer_->height());
    }

    // Draw each component — handle clip regions and draw calls
    for (auto& entry : drawEntries_) {
        switch (entry.type) {
        case DrawEntryType::ClipBegin: {
            renderer_->save(nullptr);
            // Clip to rounded or plain rect
            if (entry.borderRadius > 0.f)
                renderer_->clipRoundRect(entry.absX, entry.absY, entry.width, entry.height, entry.borderRadius);
            else
                renderer_->clipRect(entry.absX, entry.absY, entry.width, entry.height);
            // Background fill
            if (!entry.bgColor.empty()) {
                renderer_->setFillStyle(entry.bgColor);
                if (entry.borderRadius > 0.f) {
                    renderer_->beginPath(nullptr);
                    renderer_->roundRect(entry.absX, entry.absY, entry.width, entry.height, entry.borderRadius);
                    renderer_->fill(nullptr);
                } else {
                    renderer_->fillRect(entry.absX, entry.absY, entry.width, entry.height);
                }
            }
            // Border stroke
            if (!entry.borderColor.empty()) {
                renderer_->setStrokeStyle(entry.borderColor);
                renderer_->setLineWidth(entry.borderWidth);
                renderer_->beginPath(nullptr);
                if (entry.borderRadius > 0.f)
                    renderer_->roundRect(entry.absX, entry.absY, entry.width, entry.height, entry.borderRadius);
                else
                    renderer_->rect(entry.absX, entry.absY, entry.width, entry.height);
                renderer_->stroke(nullptr);
            }
            break;
        }

        case DrawEntryType::ClipEnd:
            // Restore state (pops the clip)
            renderer_->restore(nullptr);
            break;

        case DrawEntryType::Draw:
            // Translate to component position, call JS draw, restore
            renderer_->save(nullptr);
            renderer_->translate(entry.absX, entry.absY);
            if (JS_IsFunction(ctx_, entry.fn)) {
                JSValue argv[1] = { jsCanvasCtx_ };
                JSValue result = JS_Call(ctx_, entry.fn, JS_UNDEFINED, 1, argv);
                if (JS_IsException(result))
                    js_std_dump_error(ctx_);
                JS_FreeValue(ctx_, result);
            }
            renderer_->restore(nullptr);
            break;
        }
    }

    JS_SetContextOpaque(ctx_, previousOpaque);
}


// ── Mouse event helpers ───────────────────────────────────────────────────────

void QuickJSEngine::callJSMouseHandler(JSValue fn, float x, float y) {
    if (!ctx_ || JS_IsUndefined(fn)) return;
    JSValue ev = JS_NewObject(ctx_);
    JS_SetPropertyStr(ctx_, ev, "x", JS_NewFloat64(ctx_, x));
    JS_SetPropertyStr(ctx_, ev, "y", JS_NewFloat64(ctx_, y));
    JSValue argv[1] = { ev };
    JSValue result = JS_Call(ctx_, fn, JS_UNDEFINED, 1, argv);
    if (JS_IsException(result)) js_std_dump_error(ctx_);
    JS_FreeValue(ctx_, result);
    JS_FreeValue(ctx_, ev);
}

void QuickJSEngine::callJSMouseWheelHandler(JSValue fn, float dx, float dy) {
    if (!ctx_ || JS_IsUndefined(fn)) return;
    JSValue ev = JS_NewObject(ctx_);
    JS_SetPropertyStr(ctx_, ev, "deltaX", JS_NewFloat64(ctx_, dx));
    JS_SetPropertyStr(ctx_, ev, "deltaY", JS_NewFloat64(ctx_, dy));
    JSValue argv[1] = { ev };
    JSValue result = JS_Call(ctx_, fn, JS_UNDEFINED, 1, argv);
    if (JS_IsException(result)) js_std_dump_error(ctx_);
    JS_FreeValue(ctx_, result);
    JS_FreeValue(ctx_, ev);
}

QuickJSEngine::Hitbox* QuickJSEngine::hitTest(float x, float y) {
    // Walk in reverse: last pushed = topmost drawn
    for (int i = (int)hitboxes_.size() - 1; i >= 0; --i) {
        auto& hb = hitboxes_[i];
        if (x >= hb.x && x < hb.x + hb.w && y >= hb.y && y < hb.y + hb.h)
            return &hb;
    }
    return nullptr;
}

void QuickJSEngine::onMouseDown(float x, float y) {
    if (!ctx_) return;
    lastMouseX_ = x; lastMouseY_ = y;
    auto* hb = hitTest(x, y);
    if (!hb) return;
    // Dup the handlers so they survive hitboxes_ being rebuilt next frame
    if (!JS_IsUndefined(activeDragFn_))    JS_FreeValue(ctx_, activeDragFn_);
    if (!JS_IsUndefined(activeMouseUpFn_)) JS_FreeValue(ctx_, activeMouseUpFn_);
    activeDragFn_    = JS_IsUndefined(hb->onMouseDrag) ? JS_UNDEFINED : JS_DupValue(ctx_, hb->onMouseDrag);
    activeMouseUpFn_ = JS_IsUndefined(hb->onMouseUp)   ? JS_UNDEFINED : JS_DupValue(ctx_, hb->onMouseUp);
    dragOffsetX_ = hb->x;
    dragOffsetY_ = hb->y;
    dragging_ = true;
    callJSMouseHandler(hb->onMouseDown, x - hb->x, y - hb->y);
}

void QuickJSEngine::onMouseUp(float x, float y) {
    if (!ctx_) return;
    lastMouseX_ = x; lastMouseY_ = y;
    callJSMouseHandler(activeMouseUpFn_, x - dragOffsetX_, y - dragOffsetY_);
    JS_FreeValue(ctx_, activeDragFn_);    activeDragFn_    = JS_UNDEFINED;
    JS_FreeValue(ctx_, activeMouseUpFn_); activeMouseUpFn_ = JS_UNDEFINED;
    dragging_ = false;
}

void QuickJSEngine::onMouseMove(float x, float y) {
    if (!ctx_) return;
    lastMouseX_ = x; lastMouseY_ = y;
    if (dragging_)
        callJSMouseHandler(activeDragFn_, x - dragOffsetX_, y - dragOffsetY_);
}

void QuickJSEngine::onMouseWheel(float dx, float dy) {
    if (!ctx_) return;
    auto* hb = hitTest(lastMouseX_, lastMouseY_);
    if (hb)
        callJSMouseWheelHandler(hb->onMouseWheel, dx, dy);
}

JSValue QuickJSEngine::setParameter(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    int32_t parameterId;
    double parameterValue;

    JS_ToInt32(ctx, &parameterId, argv[0]);
    JS_ToFloat64(ctx, &parameterValue, argv[1]);

    parameterStore_.setParameter(parameterId, parameterValue);

    // Redraw root so all components bound to this parameter update
    // renderer_->redrawAll();

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