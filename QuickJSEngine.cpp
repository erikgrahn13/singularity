#include "QuickJSEngine.h"
// #include <print>

#if defined NDEBUG
#include <hello_release.h>
#endif

static int js_events_module_init(JSContext *ctx, JSModuleDef *m)
{
    JS_SetModuleExport(ctx, m, "addEventListener", JS_NewCFunction(ctx, QuickJSEngine::js_addEventListener, "addEventListener", 2));
    return 0;
}

static int js_parameters_module_init(JSContext *ctx, JSModuleDef *m)
{
    JS_SetModuleExport(ctx, m, "getParameter", JS_NewCFunction(ctx, QuickJSEngine::js_getParameter, "getParameter", 1));
    JS_SetModuleExport(ctx, m, "setParameter", JS_NewCFunction(ctx, QuickJSEngine::js_setParameter, "setParameter", 2));
    return 0;
}

static int js_audio_module_init(JSContext *ctx, JSModuleDef *m)
{
    JS_SetModuleExport(ctx, m, "getAudioBackends", JS_NewCFunction(ctx, QuickJSEngine::js_getAudioBackends, "getAudioBackends", 0));
    JS_SetModuleExport(ctx, m, "getStringList", JS_NewCFunction(ctx, QuickJSEngine::js_getStringList, "getStringList", 1));
    return 0;
}

static JSModuleDef *js_module_loader_custom(JSContext *ctx, const char *module_name, void *opaque, JSValueConst attributes)
{
    if (strcmp(module_name, "native:events") == 0) {
        JSModuleDef *m = JS_NewCModule(ctx, module_name, js_events_module_init);
        if (!m) return nullptr;
        JS_AddModuleExport(ctx, m, "addEventListener");
        return m;
    }
    if (strcmp(module_name, "native:parameters") == 0) {
        JSModuleDef *m = JS_NewCModule(ctx, module_name, js_parameters_module_init);
        if (!m) return nullptr;
        JS_AddModuleExport(ctx, m, "getParameter");
        JS_AddModuleExport(ctx, m, "setParameter");
        return m;
    }
    if (strcmp(module_name, "native:audio") == 0) {
        JSModuleDef *m = JS_NewCModule(ctx, module_name, js_audio_module_init);
        if (!m) return nullptr;
        JS_AddModuleExport(ctx, m, "getAudioBackends");
        JS_AddModuleExport(ctx, m, "getStringList");
        return m;
    }
    return js_module_loader(ctx, module_name, opaque, attributes);
}

std::unique_ptr<IJSEngine> createJSEngine(IRenderer *renderer, IParameterProvider &parameterStore)
{
    return std::make_unique<QuickJSEngine>(renderer, parameterStore);
}

QuickJSEngine::QuickJSEngine(IRenderer *renderer, IParameterProvider &parameterStore)
 : currentRenderer(renderer), parameterStore(parameterStore)
{
    setupJS();
}

QuickJSEngine::~QuickJSEngine()
{
    freeEventListeners();
    if(ctx) 
        JS_FreeContext(ctx);
    if(rt)
        JS_FreeRuntime(rt);
}

void QuickJSEngine::hotReload()
{
    // std::println("QuickJS Hotreload");
    setupJS();
    if (!currentScriptPath.empty())
        loadScript(currentScriptPath);
}

void QuickJSEngine::renderUI()
{
    JSValue globalObj = JS_GetGlobalObject(ctx);
    JSValue fn = JS_GetPropertyStr(ctx, globalObj, "renderUI");

    if (JS_IsFunction(ctx, fn)) {
        JSValue result = JS_Call(ctx, fn, JS_UNDEFINED, 0, nullptr);

        if (JS_IsException(result)) {
        }

        JS_FreeValue(ctx, result);
    } 

    JS_FreeValue(ctx, fn);
    JS_FreeValue(ctx, globalObj);
}

void QuickJSEngine::setupJS()
{
    currentRenderer->clear();

    freeEventListeners();
    if(ctx) 
        JS_FreeContext(ctx);
    if(rt) {
        JS_FreeRuntime(rt);
    }

    rt = JS_NewRuntime();
    gradientClassId = 0;
    JS_NewClassID(rt, &gradientClassId);
    JSClassDef gradientClass = { "CanvasGradient" };
    JS_NewClass(rt, gradientClassId, &gradientClass);

    JS_SetModuleLoaderFunc2(rt, nullptr, js_module_loader_custom, js_module_check_attributes, nullptr);
    ctx = JS_NewContext(rt);
    js_std_add_helpers(ctx, 0, nullptr);
    JS_SetContextOpaque(ctx, this);

    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue obj = JS_NewObject(ctx);
    
    // Methods
    JS_SetPropertyStr(ctx, obj, "fillRect", JS_NewCFunction(ctx, js_fillRect, "fillRect", 4));
    JS_SetPropertyStr(ctx, obj, "clearRect", JS_NewCFunction(ctx, js_fillRect, "fillRect", 4));
    JS_SetPropertyStr(ctx, obj, "strokeRect", JS_NewCFunction(ctx, js_strokeRect, "strokeRect", 4));
    JS_SetPropertyStr(ctx, obj, "roundRect", JS_NewCFunction(ctx, js_roundRect, "roundRect", 5));
    JS_SetPropertyStr(ctx, obj, "beginPath", JS_NewCFunction(ctx, js_beginPath, "beginPath", 1));
    JS_SetPropertyStr(ctx, obj, "arc", JS_NewCFunction(ctx, js_arc, "arc", 5));
    JS_SetPropertyStr(ctx, obj, "stroke", JS_NewCFunction(ctx, js_stroke, "stroke", 1));
    JS_SetPropertyStr(ctx, obj, "save", JS_NewCFunction(ctx, js_save, "save", 1));
    JS_SetPropertyStr(ctx, obj, "restore", JS_NewCFunction(ctx, js_restore, "restore", 1));
    JS_SetPropertyStr(ctx, obj, "fill", JS_NewCFunction(ctx, js_fill, "fill", 1));
    JS_SetPropertyStr(ctx, obj, "moveTo", JS_NewCFunction(ctx, js_moveTo, "moveTo", 2));
    JS_SetPropertyStr(ctx, obj, "quadraticCurveTo", JS_NewCFunction(ctx, js_quadraticCurveTo, "quadraticCurveTo", 4));
    JS_SetPropertyStr(ctx, obj, "bezierCurveTo", JS_NewCFunction(ctx, js_bezierCurveTo, "bezierCurveTo", 6));
    JS_SetPropertyStr(ctx, obj, "arcTo", JS_NewCFunction(ctx, js_arcTo, "arcTo", 5));
    JS_SetPropertyStr(ctx, obj, "ellipse", JS_NewCFunction(ctx, js_ellipse, "ellipse", 7));
    JS_SetPropertyStr(ctx, obj, "rect", JS_NewCFunction(ctx, js_rect, "rect", 4));
    JS_SetPropertyStr(ctx, obj, "fillText", JS_NewCFunction(ctx, js_fillText, "fillText", 3));
    JS_SetPropertyStr(ctx, obj, "strokeText", JS_NewCFunction(ctx, js_strokeText, "strokeText", 3));
    JS_SetPropertyStr(ctx, obj, "measureText", JS_NewCFunction(ctx, js_measureText, "measureText", 1));
    JS_SetPropertyStr(ctx, obj, "lineTo", JS_NewCFunction(ctx, js_lineTo, "lineTo", 2));
    JS_SetPropertyStr(ctx, obj, "closePath", JS_NewCFunction(ctx, js_closePath, "closePath", 1));
    JS_SetPropertyStr(ctx, obj, "rotate", JS_NewCFunction(ctx, js_rotate, "rotate", 1));
    JS_SetPropertyStr(ctx, obj, "translate", JS_NewCFunction(ctx, js_translate, "translate", 2));
    JS_SetPropertyStr(ctx, obj, "scale", JS_NewCFunction(ctx, js_scale, "scale", 2));
    JS_SetPropertyStr(ctx, obj, "resetTransform", JS_NewCFunction(ctx, js_resetTransform, "resetTransform", 1));
    JS_SetPropertyStr(ctx, obj, "createLinearGradient", JS_NewCFunction(ctx, js_createLinearGradient, "createLinearGradient", 4));
    JS_SetPropertyStr(ctx, obj, "createRadialGradient", JS_NewCFunction(ctx, js_createRadialGradient, "createRadialGradient", 6));


    // Properties
    JS_DefinePropertyGetSet(ctx, obj, JS_NewAtom(ctx, "fillStyle"), JS_UNDEFINED, JS_NewCFunction(ctx, js_fillStyle, "fillStyle", 1), JS_PROP_CONFIGURABLE);
    JS_DefinePropertyGetSet(ctx, obj, JS_NewAtom(ctx, "strokeStyle"), JS_UNDEFINED, JS_NewCFunction(ctx, js_strokeStyle, "strokeStyle", 1), JS_PROP_CONFIGURABLE);
    JS_DefinePropertyGetSet(ctx, obj, JS_NewAtom(ctx, "globalAlpha"), JS_UNDEFINED, JS_NewCFunction(ctx, js_globalAlpha, "globalAlpha", 1), JS_PROP_CONFIGURABLE);
    JS_DefinePropertyGetSet(ctx, obj, JS_NewAtom(ctx, "lineWidth"), JS_UNDEFINED, JS_NewCFunction(ctx, js_lineWidth, "lineWidth", 1), JS_PROP_CONFIGURABLE);
    JS_DefinePropertyGetSet(ctx, obj, JS_NewAtom(ctx, "lineCap"), JS_UNDEFINED, JS_NewCFunction(ctx, js_lineCap, "lineCap", 1), JS_PROP_CONFIGURABLE);
    JS_DefinePropertyGetSet(ctx, obj, JS_NewAtom(ctx, "lineJoin"), JS_UNDEFINED, JS_NewCFunction(ctx, js_lineJoin, "lineJoin", 1), JS_PROP_CONFIGURABLE);
    JS_DefinePropertyGetSet(ctx, obj, JS_NewAtom(ctx, "font"), JS_UNDEFINED, JS_NewCFunction(ctx, js_font, "font", 1), JS_PROP_CONFIGURABLE);
    JS_DefinePropertyGetSet(ctx, obj, JS_NewAtom(ctx, "textAlign"), JS_UNDEFINED, JS_NewCFunction(ctx, js_textAlign, "textAlign", 1), JS_PROP_CONFIGURABLE);
    JS_DefinePropertyGetSet(ctx, obj, JS_NewAtom(ctx, "textBaseline"), JS_UNDEFINED, JS_NewCFunction(ctx, js_textBaseline, "textBaseline", 1), JS_PROP_CONFIGURABLE);
    JS_DefinePropertyGetSet(ctx, obj, JS_NewAtom(ctx, "shadowColor"), JS_UNDEFINED, JS_NewCFunction(ctx, js_shadowColor, "shadowColor", 1), JS_PROP_CONFIGURABLE);
    JS_DefinePropertyGetSet(ctx, obj, JS_NewAtom(ctx, "shadowBlur"), JS_UNDEFINED, JS_NewCFunction(ctx, js_shadowBlur, "shadowBlur", 1), JS_PROP_CONFIGURABLE);
    JS_DefinePropertyGetSet(ctx, obj, JS_NewAtom(ctx, "shadowOffsetX"), JS_UNDEFINED, JS_NewCFunction(ctx, js_shadowOffsetX, "shadowOffsetX", 1), JS_PROP_CONFIGURABLE);
    JS_DefinePropertyGetSet(ctx, obj, JS_NewAtom(ctx, "shadowOffsetY"), JS_UNDEFINED, JS_NewCFunction(ctx, js_shadowOffsetY, "shadowOffsetY", 1), JS_PROP_CONFIGURABLE);
    
    // standalone settings window
    JS_SetPropertyStr(ctx, global_obj, "STANDALONE", JS_NewBool(ctx, true));
    JS_SetPropertyStr(ctx, global_obj, "openSettingsWindow", JS_NewCFunction(ctx, js_openSettingsWindow, "openSettingsWindow", 1));

    JSValue canvasObj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, canvasObj, "width", JS_NewInt32(ctx, currentRenderer->getWidth()));
    JS_SetPropertyStr(ctx, canvasObj, "height", JS_NewInt32(ctx, currentRenderer->getHeight()));
    JS_SetPropertyStr(ctx, obj, "canvas", canvasObj);

    JS_SetPropertyStr(ctx, global_obj, "ctx", obj);
    JS_FreeValue(ctx, global_obj);

}

void QuickJSEngine::loadScript(const std::string& path)
{
#if !defined NDEBUG
    currentScriptPath = path;
    size_t buf_len;
    auto tmp = js_load_file(ctx, &buf_len, path.c_str());
    JSValue result = JS_Eval(ctx, (const char*)tmp, buf_len, path.c_str(), JS_EVAL_TYPE_MODULE);
    js_free(ctx, tmp);
    JS_FreeValue(ctx, result);
#endif
}

void QuickJSEngine::freeEventListeners()
{
    if (!ctx) return;
    for (auto& [type, listeners] : eventListeners)
        for (auto& fn : listeners)
            JS_FreeValue(ctx, fn);
    eventListeners.clear();
}

void QuickJSEngine::dispatchEvent(const char* type, JSValue* args, int argc)
{
    auto it = eventListeners.find(type);
    if (it == eventListeners.end()) return;
    for (auto& fn : it->second) {
        JSValue result = JS_Call(ctx, fn, JS_UNDEFINED, argc, args);
        JS_FreeValue(ctx, result);
    }
}

void QuickJSEngine::onMouseDown(float x, float y)
{
    JSValue args[2] = { JS_NewFloat64(ctx, x), JS_NewFloat64(ctx, y) };
    dispatchEvent("mousedown", args, 2);
    JS_FreeValue(ctx, args[0]);
    JS_FreeValue(ctx, args[1]);
}

void QuickJSEngine::onMouseUp(float x, float y)
{
    JSValue args[2] = { JS_NewFloat64(ctx, x), JS_NewFloat64(ctx, y) };
    dispatchEvent("mouseup", args, 2);
    JS_FreeValue(ctx, args[0]);
    JS_FreeValue(ctx, args[1]);
}

void QuickJSEngine::onMouseMove(float x, float y)
{
    JSValue args[2] = { JS_NewFloat64(ctx, x), JS_NewFloat64(ctx, y) };
    dispatchEvent("mousemove", args, 2);
    JS_FreeValue(ctx, args[0]);
    JS_FreeValue(ctx, args[1]);
}

JSValue QuickJSEngine::js_addEventListener(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    auto type = JS_ToCString(ctx, argv[0]);
    self->eventListeners[type].push_back(JS_DupValue(ctx, argv[1]));
    JS_FreeCString(ctx, type);
    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_openSettingsWindow(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    if (self->onOpenSettings) self->onOpenSettings();
    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_getAudioBackends(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    JSValue arr = JS_NewArray(ctx);
    uint32_t i = 0;
    auto it = self->stringLists.find("audioBackends");
    if (it != self->stringLists.end())
        for (const auto& name : it->second)
            JS_SetPropertyUint32(ctx, arr, i++, JS_NewString(ctx, name.c_str()));
    return arr;
}

JSValue QuickJSEngine::js_getStringList(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    const char* key = JS_ToCString(ctx, argv[0]);
    JSValue arr = JS_NewArray(ctx);
    if (key) {
        uint32_t i = 0;
        auto it = self->stringLists.find(key);
        if (it != self->stringLists.end())
            for (const auto& name : it->second)
                JS_SetPropertyUint32(ctx, arr, i++, JS_NewString(ctx, name.c_str()));
        JS_FreeCString(ctx, key);
    }
    return arr;
}

JSValue QuickJSEngine::js_fillRect(JSContext *ctx, JSValue this_val, int argc, JSValue* argv)
{
    double x, y, width, height;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    JS_ToFloat64(ctx, &width, argv[2]);
    JS_ToFloat64(ctx, &height, argv[3]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);

    self->currentRenderer->fillRect(x, y, width, height);
    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_clearRect(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double x, y, width, height;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    JS_ToFloat64(ctx, &width, argv[2]);
    JS_ToFloat64(ctx, &height, argv[3]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);

    self->currentRenderer->clearRect(x, y, width, height);
    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_strokeRect(JSContext *ctx, JSValue this_val, int argc, JSValue* argv)
{
    double x, y, width, height;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    JS_ToFloat64(ctx, &width, argv[2]);
    JS_ToFloat64(ctx, &height, argv[3]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);

    self->currentRenderer->strokeRect(x, y, width, height);
    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_roundRect(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double x, y, width, height, radii;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    JS_ToFloat64(ctx, &width, argv[2]);
    JS_ToFloat64(ctx, &height, argv[3]);
    JS_ToFloat64(ctx, &radii, argv[4]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);

    self->currentRenderer->roundRect(x, y, width, height, radii);
    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_beginPath(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->beginPath();

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_arc(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double x, y, radius, startAngle, endAngle;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    JS_ToFloat64(ctx, &radius, argv[2]);
    JS_ToFloat64(ctx, &startAngle, argv[3]);
    JS_ToFloat64(ctx, &endAngle, argv[4]);

    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->arc(x, y, radius, startAngle, endAngle);

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_stroke(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->stroke();

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_save(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->save();

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_restore(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->restore();

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_fill(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->fill();

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_moveTo(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double x, y;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->moveTo(x, y);

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_lineTo(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double x, y;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->lineTo(x, y);

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_closePath(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->closePath();

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_quadraticCurveTo(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double cpx, cpy, x, y;
    JS_ToFloat64(ctx, &cpx, argv[0]);
    JS_ToFloat64(ctx, &cpy, argv[1]);
    JS_ToFloat64(ctx, &x, argv[2]);
    JS_ToFloat64(ctx, &y, argv[3]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->quadraticCurveTo(cpx, cpy, x, y);

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_bezierCurveTo(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double cp1x, cp1y, cp2x, cp2y, x, y;
    JS_ToFloat64(ctx, &cp1x, argv[0]);
    JS_ToFloat64(ctx, &cp1y, argv[1]);
    JS_ToFloat64(ctx, &cp2x, argv[2]);
    JS_ToFloat64(ctx, &cp2y, argv[3]);
    JS_ToFloat64(ctx, &x, argv[4]);
    JS_ToFloat64(ctx, &y, argv[5]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y);

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_arcTo(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double x1, y1, x2, y2, radius;
    JS_ToFloat64(ctx, &x1, argv[0]);
    JS_ToFloat64(ctx, &y1, argv[1]);
    JS_ToFloat64(ctx, &x2, argv[2]);
    JS_ToFloat64(ctx, &y2, argv[3]);
    JS_ToFloat64(ctx, &radius, argv[4]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->arcTo(x1, y1, x2, y2, radius);

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_ellipse(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double x, y, radiusX, radiusY, rotation, startAngle, endAngle;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    JS_ToFloat64(ctx, &radiusX, argv[2]);
    JS_ToFloat64(ctx, &radiusY, argv[3]);
    JS_ToFloat64(ctx, &rotation, argv[4]);
    JS_ToFloat64(ctx, &startAngle, argv[5]);
    JS_ToFloat64(ctx, &endAngle, argv[6]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->ellipse(x, y, radiusX, radiusY, rotation, startAngle, endAngle);

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_rect(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double x, y, width, height;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    JS_ToFloat64(ctx, &width, argv[2]);
    JS_ToFloat64(ctx, &height, argv[3]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->rect(x, y, width, height);

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_fillText(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double x, y;
    auto text = JS_ToCString(ctx, argv[0]);
    JS_ToFloat64(ctx, &x, argv[1]);
    JS_ToFloat64(ctx, &y, argv[2]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->fillText(text, x, y);

    JS_FreeCString(ctx, text);
    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_strokeText(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double x, y;
    auto text = JS_ToCString(ctx, argv[0]);
    JS_ToFloat64(ctx, &x, argv[1]);
    JS_ToFloat64(ctx, &y, argv[2]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->strokeText(text, x, y);

    JS_FreeCString(ctx, text);
    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_measureText(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    auto text = JS_ToCString(ctx, argv[0]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    auto width = self->currentRenderer->measureText(text);

    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "width", JS_NewFloat64(ctx, width));
    JS_FreeCString(ctx, text);
    return obj;
}

JSValue QuickJSEngine::js_rotate(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double angle;
    JS_ToFloat64(ctx, &angle, argv[0]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->rotate(angle);

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_translate(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double x, y;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->translate(x, y);

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_scale(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double x, y;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->scale(x, y);

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_resetTransform(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->resetTransform();

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_addColorStop(JSContext* ctx, JSValue this_val, int argc, JSValue* argv) {
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    int id = (int)(intptr_t)JS_GetOpaque(this_val, self->gradientClassId);

    double offset;
    JS_ToFloat64(ctx, &offset, argv[0]);
    auto color = JS_ToCString(ctx, argv[1]);

    self->currentRenderer->addColorStop(id, offset, color);

    JS_FreeCString(ctx, color);
    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_createLinearGradient(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double x0, y0, x1, y1;
    JS_ToFloat64(ctx, &x0, argv[0]);
    JS_ToFloat64(ctx, &y0, argv[1]);
    JS_ToFloat64(ctx, &x1, argv[2]);
    JS_ToFloat64(ctx, &y1, argv[3]);

    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    int id = self->currentRenderer->createLinearGradient(x0, y0, x1, y1);

    JSValue obj = JS_NewObjectClass(ctx, self->gradientClassId);
    JS_SetOpaque(obj, (void*)(intptr_t)id);
    JS_SetPropertyStr(ctx, obj, "addColorStop", JS_NewCFunction(ctx, js_addColorStop, "addColorStop", 2));
    return obj;
}

JSValue QuickJSEngine::js_createRadialGradient(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double x0, y0, r0, x1, y1, r1;
    JS_ToFloat64(ctx, &x0, argv[0]);
    JS_ToFloat64(ctx, &y0, argv[1]);
    JS_ToFloat64(ctx, &r0, argv[2]);
    JS_ToFloat64(ctx, &x1, argv[3]);
    JS_ToFloat64(ctx, &y1, argv[4]);
    JS_ToFloat64(ctx, &r1, argv[5]);

    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    int id = self->currentRenderer->createRadialGradient(x0, y0, r0, x1, y1, r1);

    JSValue obj = JS_NewObjectClass(ctx, self->gradientClassId);
    JS_SetOpaque(obj, (void*)(intptr_t)id);
    JS_SetPropertyStr(ctx, obj, "addColorStop", JS_NewCFunction(ctx, js_addColorStop, "addColorStop", 2));
    return obj;
}

JSValue QuickJSEngine::js_drawImage(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double dx, dy, dw, dh;
    auto name = JS_ToCString(ctx, argv[0]);
    JS_ToFloat64(ctx, &dx, argv[1]);
    JS_ToFloat64(ctx, &dy, argv[2]);
    JS_ToFloat64(ctx, &dw, argv[3]);
    JS_ToFloat64(ctx, &dh, argv[4]);



    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->drawImage(name, dx, dy, dw, dh);

    JS_FreeCString(ctx, name);
    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_font(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    auto font = JS_ToCString(ctx, argv[0]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->font(font);

    JS_FreeCString(ctx, font);
    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_fillStyle(JSContext *ctx, JSValue this_val, int argc, JSValue* argv)
{
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    if (JS_GetClassID(argv[0]) == self->gradientClassId) {
        int id = (int)(intptr_t)JS_GetOpaque(argv[0], self->gradientClassId);
        self->currentRenderer->setFillStyleGradient(id);
    } else {
        auto color = JS_ToCString(ctx, argv[0]);
        self->currentRenderer->setFillStyle(color);
        JS_FreeCString(ctx, color);
    }
    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_strokeStyle(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    auto color = JS_ToCString(ctx, argv[0]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->setStrokeStyle(color);

    JS_FreeCString(ctx, color);
    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_globalAlpha(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double alpha;
    JS_ToFloat64(ctx, &alpha, argv[0]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->setGlobalAlpha(alpha);

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_lineWidth(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double lineWidth;
    JS_ToFloat64(ctx, &lineWidth, argv[0]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->setLineWidth(lineWidth);

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_lineCap(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    auto cap = JS_ToCString(ctx, argv[0]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->setLineCap(cap);

    JS_FreeCString(ctx, cap);
    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_lineJoin(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    auto join = JS_ToCString(ctx, argv[0]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->setLineJoin(join);

    JS_FreeCString(ctx, join);
    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_textAlign(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    auto align = JS_ToCString(ctx, argv[0]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->textAlign(align);

    JS_FreeCString(ctx, align);
    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_textBaseline(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    auto baseline = JS_ToCString(ctx, argv[0]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->textBaseline(baseline);

    JS_FreeCString(ctx, baseline);
    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_shadowColor(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    auto color = JS_ToCString(ctx, argv[0]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->setShadowColor(color);

    JS_FreeCString(ctx, color);
    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_shadowBlur(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double blur;
    JS_ToFloat64(ctx, &blur, argv[0]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->setShadowBlur(blur);

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_shadowOffsetX(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double offsetX;
    JS_ToFloat64(ctx, &offsetX, argv[0]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->setShadowOffsetX(offsetX);

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_shadowOffsetY(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    double offsetY;
    JS_ToFloat64(ctx, &offsetY, argv[0]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->setShadowOffsetY(offsetY);

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_setParameter(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    int32_t parameterId;
    double parameterValue;

    JS_ToInt32(ctx, &parameterId, argv[0]);
    JS_ToFloat64(ctx, &parameterValue, argv[1]);

    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->parameterStore.setParameter(parameterId, parameterValue);

    return JS_UNDEFINED;
}

JSValue QuickJSEngine::js_getParameter(JSContext *ctx, JSValue this_val, int argc, JSValue *argv)
{
    int32_t parameterId;
    JS_ToInt32(ctx, &parameterId, argv[0]);

    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    double value = self->parameterStore.getParameter(parameterId);

    return JS_NewFloat64(ctx, value);
}
