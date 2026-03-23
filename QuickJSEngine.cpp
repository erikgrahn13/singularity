#include "QuickJSEngine.h"
#include <print>

std::unique_ptr<IJSEngine> createJSEngine()
{
    return std::make_unique<QuickJSEngine>();
}

QuickJSEngine::QuickJSEngine()
{
    rt = JS_NewRuntime();
    ctx =  JS_NewContext(rt);

    std::println("QuickJSEngine init {}/hello.js",JS_SCRIPTS_DIR);




}

QuickJSEngine::~QuickJSEngine()
{
    if(ctx)
        JS_FreeContext(ctx);
    if(rt)
        JS_FreeRuntime(rt);
}

void QuickJSEngine::hotReload()
{
    JS_FreeContext(ctx);
    JS_FreeRuntime(rt);

    rt = JS_NewRuntime();
    JS_SetModuleLoaderFunc2(rt, nullptr, js_module_loader, js_module_check_attributes, nullptr);
    ctx = JS_NewContext(rt);
    js_std_add_helpers(ctx, 0, nullptr);
    JS_SetContextOpaque(ctx, this);
    bindRenderer(currentRenderer);

    size_t buf_len;
    std::string path = JS_SCRIPTS_DIR"/hello.js";
    auto tmp = js_load_file(ctx, &buf_len, path.c_str());
    JSValue result = JS_Eval(ctx, (const char*)tmp, buf_len, path.c_str(), JS_EVAL_TYPE_MODULE);

    js_free(ctx, tmp);
    JS_FreeValue(ctx, result);
}

void QuickJSEngine::bindRenderer(IRenderer *renderer)
{
    currentRenderer = renderer;
    std::println("bindRenderer called");

    JSValue global_obj = JS_GetGlobalObject(ctx);
    JSValue obj = JS_NewObject(ctx);
    
    // Methods
    JS_SetPropertyStr(ctx, obj, "fillRect", JS_NewCFunction(ctx, js_fillRect, "fillRect", 4));
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

    // Properties
    JS_DefinePropertyGetSet(ctx, obj, JS_NewAtom(ctx, "fillStyle"), JS_UNDEFINED, JS_NewCFunction(ctx, js_fillStyle, "fillStyle", 1), JS_PROP_CONFIGURABLE);
    JS_DefinePropertyGetSet(ctx, obj, JS_NewAtom(ctx, "strokeStyle"), JS_UNDEFINED, JS_NewCFunction(ctx, js_strokeStyle, "strokeStyle", 1), JS_PROP_CONFIGURABLE);
    JS_DefinePropertyGetSet(ctx, obj, JS_NewAtom(ctx, "globalAlpha"), JS_UNDEFINED, JS_NewCFunction(ctx, js_globalAlpha, "globalAlpha", 1), JS_PROP_CONFIGURABLE);
    JS_DefinePropertyGetSet(ctx, obj, JS_NewAtom(ctx, "lineWidth"), JS_UNDEFINED, JS_NewCFunction(ctx, js_lineWidth, "lineWidth", 1), JS_PROP_CONFIGURABLE);
    JS_DefinePropertyGetSet(ctx, obj, JS_NewAtom(ctx, "lineCap"), JS_UNDEFINED, JS_NewCFunction(ctx, js_lineCap, "lineCap", 1), JS_PROP_CONFIGURABLE);
    JS_DefinePropertyGetSet(ctx, obj, JS_NewAtom(ctx, "lineJoin"), JS_UNDEFINED, JS_NewCFunction(ctx, js_lineJoin, "lineJoin", 1), JS_PROP_CONFIGURABLE);
    
    JS_SetPropertyStr(ctx, global_obj, "ctx", obj);
    JS_FreeValue(ctx, global_obj);
}

void QuickJSEngine::onMouseDown(float x, float y)
{
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
    self->currentRenderer->fill();

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

JSValue QuickJSEngine::js_fillStyle(JSContext *ctx, JSValue this_val, int argc, JSValue* argv)
{
    auto color = JS_ToCString(ctx, argv[0]);
    auto* self = (QuickJSEngine*)JS_GetContextOpaque(ctx);
    self->currentRenderer->setFillStyle(color);

    JS_FreeCString(ctx, color);
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
