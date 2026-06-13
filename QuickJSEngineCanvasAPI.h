#pragma once
#include "quickjs-libc.h"
#include "IRenderer2.h"

struct DrawContextData {
    IRenderer* renderer;
    void* canvas;
    void* component; // The Frame/ApplicationWindow for redraw
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
    data->renderer->fillRect((float)x, (float)y, (float)width, (float)height);
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
    data->renderer->strokeRect((float)x, (float)y, (float)width, (float)height);
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
    data->renderer->clearRect((float)x, (float)y, (float)width, (float)height);
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
    data->renderer->moveTo((float)x, (float)y);
    return JS_UNDEFINED;
}

static JSValue js_lineTo(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double x = 0, y = 0;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->lineTo((float)x, (float)y);
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
    data->renderer->arc((float)cx, (float)cy, (float)r, (float)start, (float)end, ccw);
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
    data->renderer->arcTo((float)x1, (float)y1, (float)x2, (float)y2, (float)radius);
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
    data->renderer->quadraticCurveTo((float)cpx, (float)cpy, (float)x, (float)y);
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
    data->renderer->bezierCurveTo((float)cp1x, (float)cp1y,
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
    data->renderer->ellipse((float)cx, (float)cy, (float)rx, (float)ry,
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
    data->renderer->rect((float)x, (float)y, (float)w, (float)h);
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
    data->renderer->roundRect((float)x, (float)y, (float)w, (float)h, (float)r);
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
        data->renderer->fillText(str, (float)x, (float)y);
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
        data->renderer->strokeText(str, (float)x, (float)y);
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
        w = data->renderer->measureText(str);
        JS_FreeCString(ctx, str);
    }
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "width", JS_NewFloat64(ctx, w));
    return obj;
}

static JSValue js_addColorStop(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    JSValue idVal = JS_GetPropertyStr(ctx, this_val, "_id");
    int id = -1;
    if (!JS_IsUndefined(idVal)) {
        JS_ToInt32(ctx, &id, idVal);
    }
    JS_FreeValue(ctx, idVal);
    if (id < 0) return JS_ThrowTypeError(ctx, "addColorStop called on invalid gradient");

    double offset = 0.0;
    JS_ToFloat64(ctx, &offset, argv[0]);
    const char* color = JS_ToCString(ctx, argv[1]);
    // Optional 3rd argument: per-stop HDR multiplier (default 1.0).
    // Allows matching Visage's per-gradient-stop HDR (e.g. rainbow * boost).
    double hdr = 1.0;
    if (argc >= 3 && !JS_IsUndefined(argv[2])) {
        JS_ToFloat64(ctx, &hdr, argv[2]);
    }
    if (color) {
        data->renderer->addColorStop(id, (float)offset, color, (float)hdr);
        JS_FreeCString(ctx, color);
    }
    return JS_UNDEFINED;
}

static JSValue js_createLinearGradient(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double x0 = 0, y0 = 0, x1 = 0, y1 = 0;
    JS_ToFloat64(ctx, &x0, argv[0]);
    JS_ToFloat64(ctx, &y0, argv[1]);
    JS_ToFloat64(ctx, &x1, argv[2]);
    JS_ToFloat64(ctx, &y1, argv[3]);

    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    int id = data->renderer->createLinearGradient((float)x0, (float)y0, (float)x1, (float)y1);

    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "_id", JS_NewInt32(ctx, id));
    JS_SetPropertyStr(ctx, obj, "addColorStop", JS_NewCFunction(ctx, js_addColorStop, "addColorStop", 2));
    return obj;
}

static JSValue js_createRadialGradient(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double x0 = 0, y0 = 0, r0 = 0, x1 = 0, y1 = 0, r1 = 0;
    JS_ToFloat64(ctx, &x0, argv[0]);
    JS_ToFloat64(ctx, &y0, argv[1]);
    JS_ToFloat64(ctx, &r0, argv[2]);
    JS_ToFloat64(ctx, &x1, argv[3]);
    JS_ToFloat64(ctx, &y1, argv[4]);
    JS_ToFloat64(ctx, &r1, argv[5]);

    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    int id = data->renderer->createRadialGradient((float)x0, (float)y0, (float)r0, (float)x1, (float)y1, (float)r1);

    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "_id", JS_NewInt32(ctx, id));
    JS_SetPropertyStr(ctx, obj, "addColorStop", JS_NewCFunction(ctx, js_addColorStop, "addColorStop", 2));
    return obj;
}

// ---------------------------------------------------------------------------
// Style property setters
// ---------------------------------------------------------------------------

static JSValue js_fillStyle(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    // If passed an object with an _id property, treat it as a gradient
    if (JS_IsObject(argv[0])) {
        JSValue idVal = JS_GetPropertyStr(ctx, argv[0], "_id");
        if (!JS_IsUndefined(idVal) && JS_IsNumber(idVal)) {
            int id = 0; JS_ToInt32(ctx, &id, idVal);
            JS_FreeValue(ctx, idVal);
            data->renderer->setFillStyleGradient(id);
            return JS_UNDEFINED;
        }
        JS_FreeValue(ctx, idVal);
    }

    const char* color = JS_ToCString(ctx, argv[0]);
    if (color) {
        data->fillColor = color;
        data->renderer->setFillStyle(color);
        JS_FreeCString(ctx, color);
    }
    return JS_UNDEFINED;
}

static JSValue js_strokeStyle(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    // Gradient object?
    if (JS_IsObject(argv[0])) {
        JSValue idVal = JS_GetPropertyStr(ctx, argv[0], "_id");
        if (!JS_IsUndefined(idVal) && JS_IsNumber(idVal)) {
            int id = 0; JS_ToInt32(ctx, &id, idVal);
            JS_FreeValue(ctx, idVal);
            data->renderer->setStrokeStyleGradient(id);
            return JS_UNDEFINED;
        }
        JS_FreeValue(ctx, idVal);
    }

    const char* color = JS_ToCString(ctx, argv[0]);
    if (color) {
        data->renderer->setStrokeStyle(color);
        JS_FreeCString(ctx, color);
    }
    return JS_UNDEFINED;
}

static JSValue js_lineWidth(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double w = 1.0;
    JS_ToFloat64(ctx, &w, argv[0]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->setLineWidth((float)w);
    return JS_UNDEFINED;
}

static JSValue js_lineCap(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    const char* cap = JS_ToCString(ctx, argv[0]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    if (cap) {
        data->renderer->setLineCap(cap);
        JS_FreeCString(ctx, cap);
    }
    return JS_UNDEFINED;
}

static JSValue js_font(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    const char* font = JS_ToCString(ctx, argv[0]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    if (font) {
        data->renderer->setFont(font);
        JS_FreeCString(ctx, font);
    }
    return JS_UNDEFINED;
}

static JSValue js_globalAlpha(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double a = 1.0;
    JS_ToFloat64(ctx, &a, argv[0]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->setGlobalAlpha((float)a);
    return JS_UNDEFINED;
}

static JSValue js_textAlign(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    const char* align = JS_ToCString(ctx, argv[0]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    if (align) {
        data->renderer->setTextAlign(align);
        JS_FreeCString(ctx, align);
    }
    return JS_UNDEFINED;
}

static JSValue js_textBaseline(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    const char* baseline = JS_ToCString(ctx, argv[0]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    if (baseline) {
        data->renderer->setTextBaseline(baseline);
        JS_FreeCString(ctx, baseline);
    }
    return JS_UNDEFINED;
}

// ctx.hdrMultiplier = 2.0  — brightness factor >1 makes colors over-bright, driving bloom/glow.
// Reset to 1.0 with ctx.restore() or by assigning ctx.hdrMultiplier = 1.
static JSValue js_hdrMultiplier(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double mult = 1.0;
    JS_ToFloat64(ctx, &mult, argv[0]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->setHdrMultiplier(static_cast<float>(mult));
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
    data->renderer->translate((float)x, (float)y);
    return JS_UNDEFINED;
}

static JSValue js_rotate(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double angle = 0;
    JS_ToFloat64(ctx, &angle, argv[0]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->rotate((float)angle);
    return JS_UNDEFINED;
}

static JSValue js_scale(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    double x = 1, y = 1;
    JS_ToFloat64(ctx, &x, argv[0]);
    JS_ToFloat64(ctx, &y, argv[1]);
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->scale((float)x, (float)y);
    return JS_UNDEFINED;
}

static JSValue js_resetTransform(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->resetTransform(data->canvas);
    return JS_UNDEFINED;
}

static JSValue js_drawImage(JSContext* ctx, JSValueConst this_val, int argc, JSValueConst* argv)
{
    if (argc < 5)
        return JS_ThrowTypeError(ctx, "drawImage requires 5 arguments (name, dx, dy, dw, dh)");

    const char* name = JS_ToCString(ctx, argv[0]);
    if (!name)
        return JS_ThrowTypeError(ctx, "drawImage: first argument must be a string");

    double dx = 0, dy = 0, dw = 0, dh = 0;
    JS_ToFloat64(ctx, &dx, argv[1]);
    JS_ToFloat64(ctx, &dy, argv[2]);
    JS_ToFloat64(ctx, &dw, argv[3]);
    JS_ToFloat64(ctx, &dh, argv[4]);

    auto* data = static_cast<DrawContextData*>(JS_GetContextOpaque(ctx));
    data->renderer->drawImage(name, (float)dx, (float)dy, (float)dw, (float)dh);

    JS_FreeCString(ctx, name);
    return JS_UNDEFINED;
}