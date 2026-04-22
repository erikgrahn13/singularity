#pragma once
#include "quickjs-libc.h"
#include "IRenderer2.h"

struct DrawContextData {
    IRenderer* renderer;
    void* canvas;
    std::string fillColor;
};

// ---------------------------------------------------------------------------
// Rect helpers
// ---------------------------------------------------------------------------

static JSValue js_fillRect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double x = 0, y = 0, width = 0, height = 0;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    JS_ToFloat64(ctx, &width, argv[2]);
    JS_ToFloat64(ctx, &height, argv[3]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->fillRect(data->canvas, (float)x, (float)y, (float)width, (float)height);
    return JS_UNDEFINED;
}

static JSValue js_strokeRect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double x = 0, y = 0, width = 0, height = 0;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    JS_ToFloat64(ctx, &width, argv[2]);
    JS_ToFloat64(ctx, &height, argv[3]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->strokeRect(data->canvas, (float)x, (float)y, (float)width, (float)height);
    return JS_UNDEFINED;
}

static JSValue js_clearRect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double x = 0, y = 0, width = 0, height = 0;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    JS_ToFloat64(ctx, &width, argv[2]);
    JS_ToFloat64(ctx, &height, argv[3]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->clearRect(data->canvas, (float)x, (float)y, (float)width, (float)height);
    return JS_UNDEFINED;
}

// ---------------------------------------------------------------------------
// Path building
// ---------------------------------------------------------------------------

static JSValue js_beginPath(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->beginPath(data->canvas);
    return JS_UNDEFINED;
}

static JSValue js_moveTo(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double x = 0, y = 0;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->moveTo(data->canvas, (float)x, (float)y);
    return JS_UNDEFINED;
}

static JSValue js_lineTo(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double x = 0, y = 0;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->lineTo(data->canvas, (float)x, (float)y);
    return JS_UNDEFINED;
}

static JSValue js_closePath(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->closePath(data->canvas);
    return JS_UNDEFINED;
}

static JSValue js_arc(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double cx = 0, cy = 0, r = 0, start = 0, end = 0;
    JS_ToFloat64(ctx, &cx, argv[0]);
    JS_ToFloat64(ctx, &cy, argv[1]);
    JS_ToFloat64(ctx, &r, argv[2]);
    JS_ToFloat64(ctx, &start, argv[3]);
    JS_ToFloat64(ctx, &end, argv[4]);
    bool ccw = (argc >= 6 && JS_ToBool(ctx, argv[5]));
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->arc(data->canvas, (float)cx, (float)cy, (float)r, (float)start, (float)end, ccw);
    return JS_UNDEFINED;
}

static JSValue js_arcTo(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double x1 = 0, y1 = 0, x2 = 0, y2 = 0, radius = 0;
    JS_ToFloat64(ctx, &x1, argv[0]);
    JS_ToFloat64(ctx, &y1, argv[1]);
    JS_ToFloat64(ctx, &x2, argv[2]);
    JS_ToFloat64(ctx, &y2, argv[3]);
    JS_ToFloat64(ctx, &radius, argv[4]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->arcTo(data->canvas, (float)x1, (float)y1, (float)x2, (float)y2, (float)radius);
    return JS_UNDEFINED;
}

static JSValue js_quadraticCurveTo(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double cpx = 0, cpy = 0, x = 0, y = 0;
    JS_ToFloat64(ctx, &cpx, argv[0]);
    JS_ToFloat64(ctx, &cpy, argv[1]);
    JS_ToFloat64(ctx, &x, argv[2]);
    JS_ToFloat64(ctx, &y, argv[3]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->quadraticCurveTo(data->canvas, (float)cpx, (float)cpy, (float)x, (float)y);
    return JS_UNDEFINED;
}

static JSValue js_bezierCurveTo(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double cp1x = 0, cp1y = 0, cp2x = 0, cp2y = 0, x = 0, y = 0;
    JS_ToFloat64(ctx, &cp1x, argv[0]);
    JS_ToFloat64(ctx, &cp1y, argv[1]);
    JS_ToFloat64(ctx, &cp2x, argv[2]);
    JS_ToFloat64(ctx, &cp2y, argv[3]);
    JS_ToFloat64(ctx, &x, argv[4]);
    JS_ToFloat64(ctx, &y, argv[5]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->bezierCurveTo(data->canvas, (float)cp1x, (float)cp1y,
                                   (float)cp2x, (float)cp2y, (float)x, (float)y);
    return JS_UNDEFINED;
}

static JSValue js_ellipse(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double cx = 0, cy = 0, rx = 0, ry = 0, rot = 0, start = 0, end = 0;
    JS_ToFloat64(ctx, &cx, argv[0]);
    JS_ToFloat64(ctx, &cy, argv[1]);
    JS_ToFloat64(ctx, &rx, argv[2]);
    JS_ToFloat64(ctx, &ry, argv[3]);
    JS_ToFloat64(ctx, &rot, argv[4]);
    JS_ToFloat64(ctx, &start, argv[5]);
    JS_ToFloat64(ctx, &end, argv[6]);
    bool ccw = (argc >= 8 && JS_ToBool(ctx, argv[7]));
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->ellipse(data->canvas, (float)cx, (float)cy, (float)rx, (float)ry,
                             (float)rot, (float)start, (float)end, ccw);
    return JS_UNDEFINED;
}

static JSValue js_rect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double x = 0, y = 0, w = 0, h = 0;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    JS_ToFloat64(ctx, &w, argv[2]);
    JS_ToFloat64(ctx, &h, argv[3]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->rect(data->canvas, (float)x, (float)y, (float)w, (float)h);
    return JS_UNDEFINED;
}

static JSValue js_roundRect(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double x = 0, y = 0, w = 0, h = 0, r = 0;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    JS_ToFloat64(ctx, &w, argv[2]);
    JS_ToFloat64(ctx, &h, argv[3]);
    if (argc >= 5) JS_ToFloat64(ctx, &r, argv[4]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->roundRect(data->canvas, (float)x, (float)y, (float)w, (float)h, (float)r);
    return JS_UNDEFINED;
}

// ---------------------------------------------------------------------------
// Path rendering
// ---------------------------------------------------------------------------

static JSValue js_fill(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->fill(data->canvas);
    return JS_UNDEFINED;
}

static JSValue js_stroke(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->stroke(data->canvas);
    return JS_UNDEFINED;
}

// ---------------------------------------------------------------------------
// Text
// ---------------------------------------------------------------------------

static JSValue js_fillText(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    const char* str = JS_ToCString(ctx, argv[0]);
    double x = 0, y = 0;
    JS_ToFloat64(ctx, &x, argv[1]);
    JS_ToFloat64(ctx, &y, argv[2]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    if (str) {
        data->renderer->fillText(data->canvas, str, (float)x, (float)y);
        JS_FreeCString(ctx, str);
    }
    return JS_UNDEFINED;
}

static JSValue js_strokeText(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    const char* str = JS_ToCString(ctx, argv[0]);
    double x = 0, y = 0;
    JS_ToFloat64(ctx, &x, argv[1]);
    JS_ToFloat64(ctx, &y, argv[2]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    if (str) {
        data->renderer->strokeText(data->canvas, str, (float)x, (float)y);
        JS_FreeCString(ctx, str);
    }
    return JS_UNDEFINED;
}

static JSValue js_measureText(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    const char* str = JS_ToCString(ctx, argv[0]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    float w = 0.0f;
    if (str) {
        w = data->renderer->measureText(data->canvas, str);
        JS_FreeCString(ctx, str);
    }
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "width", JS_NewFloat64(ctx, w));
    return obj;
}

// ---------------------------------------------------------------------------
// Style property setters
// ---------------------------------------------------------------------------

static JSValue js_fillStyle(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    const char* color = JS_ToCString(ctx, argv[0]);
    if (color) {
        data->fillColor = color;
        data->renderer->setFillStyle(data->canvas, color);
        JS_FreeCString(ctx, color);
    }
    return JS_UNDEFINED;
}

static JSValue js_strokeStyle(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    const char* color = JS_ToCString(ctx, argv[0]);
    if (color) {
        data->renderer->setStrokeStyle(data->canvas, color);
        JS_FreeCString(ctx, color);
    }
    return JS_UNDEFINED;
}

static JSValue js_lineWidth(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double w = 1.0;
    JS_ToFloat64(ctx, &w, argv[0]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->setLineWidth(data->canvas, (float)w);
    return JS_UNDEFINED;
}

static JSValue js_font(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    const char* font = JS_ToCString(ctx, argv[0]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    if (font) {
        data->renderer->setFont(data->canvas, font);
        JS_FreeCString(ctx, font);
    }
    return JS_UNDEFINED;
}

static JSValue js_globalAlpha(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double a = 1.0;
    JS_ToFloat64(ctx, &a, argv[0]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->setGlobalAlpha(data->canvas, (float)a);
    return JS_UNDEFINED;
}

static JSValue js_textAlign(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    const char* align = JS_ToCString(ctx, argv[0]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    if (align) {
        data->renderer->setTextAlign(data->canvas, align);
        JS_FreeCString(ctx, align);
    }
    return JS_UNDEFINED;
}

static JSValue js_textBaseline(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    const char* baseline = JS_ToCString(ctx, argv[0]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    if (baseline) {
        data->renderer->setTextBaseline(data->canvas, baseline);
        JS_FreeCString(ctx, baseline);
    }
    return JS_UNDEFINED;
}

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

static JSValue js_save(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->save(data->canvas);
    return JS_UNDEFINED;
}

static JSValue js_restore(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->restore(data->canvas);
    return JS_UNDEFINED;
}

// ---------------------------------------------------------------------------
// Transforms
// ---------------------------------------------------------------------------

static JSValue js_translate(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double x = 0, y = 0;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->translate(data->canvas, (float)x, (float)y);
    return JS_UNDEFINED;
}

static JSValue js_rotate(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double angle = 0;
    JS_ToFloat64(ctx, &angle, argv[0]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->rotate(data->canvas, (float)angle);
    return JS_UNDEFINED;
}

static JSValue js_scale(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double x = 1, y = 1;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->scale(data->canvas, (float)x, (float)y);
    return JS_UNDEFINED;
}

static JSValue js_resetTransform(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->resetTransform(data->canvas);
    return JS_UNDEFINED;
}