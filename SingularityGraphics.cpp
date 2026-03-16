// #define DMON_IMPL
// #include "dmon.h"
#include "SingularityGraphics.h"
#include "include/core/SkRect.h"
#include "include/core/SkColor.h"
#include "include/core/SkBlendMode.h"
#include "include/core/SkFont.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkFontStyle.h"
#ifdef _WIN32
#  include "include/ports/SkTypeface_win.h"
#endif
#include <iostream>
#include <cstdio>
#include <cmath>
#include <numbers>
#include <unordered_map>
#include <algorithm>
#include <cctype>

#include "dmonFileWatcher.h"

static SkColor parseColor(const std::string& s)
{
    if (s.empty()) return SK_ColorBLACK;

    if (s[0] == '#') {
        std::string hex = s.substr(1);
        if (hex.size() == 3)
            hex = { hex[0], hex[0], hex[1], hex[1], hex[2], hex[2] };
        if (hex.size() == 6) {
            unsigned int rgb = (unsigned int)std::stoul(hex, nullptr, 16);
            return SkColorSetARGB(0xFF, (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
        } else if (hex.size() == 8) {
            unsigned long rgba = std::stoul(hex, nullptr, 16);
            return SkColorSetARGB(rgba & 0xFF, (rgba >> 24) & 0xFF, (rgba >> 16) & 0xFF, (rgba >> 8) & 0xFF);
        }
        return SK_ColorBLACK;
    }

    static const std::unordered_map<std::string, SkColor> namedColors = {
        { "black",       SkColorSetARGB(0xFF, 0x00, 0x00, 0x00) },
        { "white",       SkColorSetARGB(0xFF, 0xFF, 0xFF, 0xFF) },
        { "red",         SkColorSetARGB(0xFF, 0xFF, 0x00, 0x00) },
        { "lime",        SkColorSetARGB(0xFF, 0x00, 0xFF, 0x00) },
        { "blue",        SkColorSetARGB(0xFF, 0x00, 0x00, 0xFF) },
        { "yellow",      SkColorSetARGB(0xFF, 0xFF, 0xFF, 0x00) },
        { "cyan",        SkColorSetARGB(0xFF, 0x00, 0xFF, 0xFF) },
        { "magenta",     SkColorSetARGB(0xFF, 0xFF, 0x00, 0xFF) },
        { "silver",      SkColorSetARGB(0xFF, 0xC0, 0xC0, 0xC0) },
        { "gray",        SkColorSetARGB(0xFF, 0x80, 0x80, 0x80) },
        { "grey",        SkColorSetARGB(0xFF, 0x80, 0x80, 0x80) },
        { "maroon",      SkColorSetARGB(0xFF, 0x80, 0x00, 0x00) },
        { "green",       SkColorSetARGB(0xFF, 0x00, 0x80, 0x00) },
        { "purple",      SkColorSetARGB(0xFF, 0x80, 0x00, 0x80) },
        { "orange",      SkColorSetARGB(0xFF, 0xFF, 0xA5, 0x00) },
        { "pink",        SkColorSetARGB(0xFF, 0xFF, 0xC0, 0xCB) },
        { "hotpink",     SkColorSetARGB(0xFF, 0xFF, 0x69, 0xB4) },
        { "coral",       SkColorSetARGB(0xFF, 0xFF, 0x7F, 0x50) },
        { "gold",        SkColorSetARGB(0xFF, 0xFF, 0xD7, 0x00) },
        { "violet",      SkColorSetARGB(0xFF, 0xEE, 0x82, 0xEE) },
        { "indigo",      SkColorSetARGB(0xFF, 0x4B, 0x00, 0x82) },
        { "brown",       SkColorSetARGB(0xFF, 0xA5, 0x2A, 0x2A) },
        { "turquoise",   SkColorSetARGB(0xFF, 0x40, 0xE0, 0xD0) },
        { "skyblue",     SkColorSetARGB(0xFF, 0x87, 0xCE, 0xEB) },
        { "transparent", SkColorSetARGB(0x00, 0x00, 0x00, 0x00) },
    };

    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(),
        [](unsigned char c) { return std::tolower(c); });
    auto it = namedColors.find(lower);
    if (it != namedColors.end()) return it->second;

    std::cerr << "[fillStyle] Unknown color: " << s << "\n";
    return SK_ColorBLACK;
}

static std::string colorToHex(SkColor c)
{
    char buf[8];
    std::snprintf(buf, sizeof(buf), "#%02x%02x%02x",
        SkColorGetR(c), SkColorGetG(c), SkColorGetB(c));
    return buf;
}

// Apply globalAlpha to a color
static SkColor applyAlpha(SkColor c, float alpha)
{
    uint8_t a = (uint8_t)(SkColorGetA(c) * alpha);
    return SkColorSetA(c, a);
}

SkPaint SingularityGraphics::makeFillPaint() const
{
    SkPaint p;
    p.setAntiAlias(true);
    p.setStyle(SkPaint::kFill_Style);
    p.setColor(applyAlpha(fillStyle, globalAlpha));
    return p;
}

SkPaint SingularityGraphics::makeStrokePaint() const
{
    SkPaint p;
    p.setAntiAlias(true);
    p.setStyle(SkPaint::kStroke_Style);
    p.setColor(applyAlpha(strokeStyle, globalAlpha));
    p.setStrokeWidth(lineWidth);
    p.setStrokeCap(lineCap);
    p.setStrokeJoin(lineJoin);
    return p;
}

// void SingularityGraphics::watch_callback(dmon_watch_id watch_id, dmon_action action, const char* rootdir,
//                                          const char* filepath, const char* oldfilepath, void* user)
// {
//     if (action == DMON_ACTION_MODIFY) {
//         std::cout << "Modified: " << filepath << "\n";
//         SingularityGraphics* self = (SingularityGraphics*)user;
//         if (self) {
//             self->scriptDirty = true;
//             if (self->onFileChanged) self->onFileChanged();
//         }
//     }
// }

SingularityGraphics::SingularityGraphics()
    : SingularityGraphics(JS_SCRIPTS_DIR "/hello.js")
{}

void SingularityGraphics::setupJS()
{
    // Tear down existing runtime. Modules are cached per-runtime, so we
    // recreate it entirely to ensure hot-reload always re-executes all imports.
    if (ctx) { JS_FreeContext(ctx); ctx = nullptr; }
    if (rt)  { JS_FreeRuntime(rt);  rt  = nullptr; }

    // Reset canvas
    SkCanvas* canvas = skiaSurface->getCanvas();
    canvas->restoreToCount(1);
    canvas->drawColor(SK_ColorGREEN);

    // Reset draw state
    fillStyle   = SK_ColorBLACK;
    strokeStyle = SK_ColorBLACK;
    lineWidth   = 1.0f;
    globalAlpha = 1.0f;
    fontSize    = 12.0f;
    lineCap     = SkPaint::kRound_Cap;
    lineJoin    = SkPaint::kRound_Join;
    currentPath = SkPathBuilder();
    stateStack.clear();

    // Create runtime + register ES module loader
    rt = JS_NewRuntime();
    JS_SetModuleLoaderFunc2(rt, nullptr, js_module_loader, js_module_check_attributes, nullptr);
    ctx = JS_NewContext(rt);

    js_std_add_helpers(ctx, 0, nullptr);
    js_init_module_std(ctx, "std");
    JS_SetContextOpaque(ctx, this);

    JSValue global = JS_GetGlobalObject(ctx);

    JS_SetPropertyStr(ctx, global, "hello",
        JS_NewCFunction(ctx, js_hello, "hello", 0));

    // Build the ctx drawing object
    JSValue ctx_obj = JS_NewObject(ctx);

    // helper lambda to register a getter/setter property
    auto defProp = [&](const char* name,
                       JSCFunction* getter, JSCFunction* setter) {
        JSAtom atom = JS_NewAtom(ctx, name);
        char gname[64], sname[64];
        std::snprintf(gname, sizeof(gname), "get %s", name);
        std::snprintf(sname, sizeof(sname), "set %s", name);
        JS_DefinePropertyGetSet(ctx, ctx_obj, atom,
            JS_NewCFunction(ctx, getter, gname, 0),
            JS_NewCFunction(ctx, setter, sname, 1),
            JS_PROP_CONFIGURABLE);
        JS_FreeAtom(ctx, atom);
    };

    // methods
    JS_SetPropertyStr(ctx, ctx_obj, "fillRect",         JS_NewCFunction(ctx, js_fillRect,         "fillRect",         4));
    JS_SetPropertyStr(ctx, ctx_obj, "strokeRect",       JS_NewCFunction(ctx, js_strokeRect,       "strokeRect",       4));
    JS_SetPropertyStr(ctx, ctx_obj, "clearRect",        JS_NewCFunction(ctx, js_clearRect,        "clearRect",        4));
    JS_SetPropertyStr(ctx, ctx_obj, "beginPath",        JS_NewCFunction(ctx, js_beginPath,        "beginPath",        0));
    JS_SetPropertyStr(ctx, ctx_obj, "closePath",        JS_NewCFunction(ctx, js_closePath,        "closePath",        0));
    JS_SetPropertyStr(ctx, ctx_obj, "moveTo",           JS_NewCFunction(ctx, js_moveTo,           "moveTo",           2));
    JS_SetPropertyStr(ctx, ctx_obj, "lineTo",           JS_NewCFunction(ctx, js_lineTo,           "lineTo",           2));
    JS_SetPropertyStr(ctx, ctx_obj, "arc",              JS_NewCFunction(ctx, js_arc,              "arc",              5));
    JS_SetPropertyStr(ctx, ctx_obj, "bezierCurveTo",    JS_NewCFunction(ctx, js_bezierCurveTo,    "bezierCurveTo",    6));
    JS_SetPropertyStr(ctx, ctx_obj, "quadraticCurveTo", JS_NewCFunction(ctx, js_quadraticCurveTo, "quadraticCurveTo", 4));
    JS_SetPropertyStr(ctx, ctx_obj, "fill",             JS_NewCFunction(ctx, js_fill,             "fill",             0));
    JS_SetPropertyStr(ctx, ctx_obj, "stroke",           JS_NewCFunction(ctx, js_stroke,           "stroke",           0));
    JS_SetPropertyStr(ctx, ctx_obj, "save",             JS_NewCFunction(ctx, js_save,             "save",             0));
    JS_SetPropertyStr(ctx, ctx_obj, "restore",          JS_NewCFunction(ctx, js_restore,          "restore",          0));
    JS_SetPropertyStr(ctx, ctx_obj, "translate",        JS_NewCFunction(ctx, js_translate,        "translate",        2));
    JS_SetPropertyStr(ctx, ctx_obj, "rotate",           JS_NewCFunction(ctx, js_rotate,           "rotate",           1));
    JS_SetPropertyStr(ctx, ctx_obj, "scale",            JS_NewCFunction(ctx, js_scale,            "scale",            2));

    JS_SetPropertyStr(ctx, ctx_obj, "fillText",          JS_NewCFunction(ctx, js_fillText,          "fillText",          3));

    // properties
    defProp("fillStyle",   js_getFillStyle,   js_setFillStyle);
    defProp("strokeStyle", js_getStrokeStyle, js_setStrokeStyle);
    defProp("lineWidth",   js_getLineWidth,   js_setLineWidth);
    defProp("globalAlpha", js_getGlobalAlpha, js_setGlobalAlpha);
    defProp("lineCap",     js_getLineCap,     js_setLineCap);
    defProp("fontSize",    js_getFontSize,    js_setFontSize);

    JS_SetPropertyStr(ctx, global, "ctx", ctx_obj);
    JS_FreeValue(ctx, global);

    // Evaluate the entry point as an ES module (imports resolved via module loader)
    if (!jsPath.empty()) {
        size_t buf_len = 0;
        uint8_t* buf = js_load_file(ctx, &buf_len, jsPath.c_str());
        if (buf) {
            JSValue result = JS_Eval(ctx, (const char*)buf, buf_len, jsPath.c_str(), JS_EVAL_TYPE_MODULE);
            if (JS_IsException(result))
                js_std_dump_error(ctx);
            JS_FreeValue(ctx, result);
            js_free(ctx, buf);
            // drain any pending microtasks created during module evaluation
            JSContext* pjCtx;
            while (JS_ExecutePendingJob(rt, &pjCtx) > 0) {}
        } else {
            std::cerr << "[setupJS] Could not load: " << jsPath << "\n";
        }
    }
}

SingularityGraphics::SingularityGraphics(const std::string& jsPath)
    : jsPath(jsPath)
{
    SkImageInfo info = SkImageInfo::Make(300, 200, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
    skiaSurface = SkSurfaces::Raster(info);

    // Acquire the platform default typeface once — used by fillText
#ifdef _WIN32
    {
        auto mgr = SkFontMgr_New_DirectWrite();
        defaultTypeface = mgr->legacyMakeTypeface(nullptr, SkFontStyle());
    }
#endif

    setupJS();

    watcher = std::make_unique<DmonFileWatcher>(JS_SCRIPTS_DIR,
        [this](const std::string& path) {
        scriptDirty = true;
        if (onFileChanged) onFileChanged();
    });
    // dmon_init();
    // dmon_watch(JS_SCRIPTS_DIR, watch_callback, DMON_WATCHFLAGS_RECURSIVE, this);
}

SingularityGraphics::SingularityGraphics(SingularityGraphics&& other) noexcept
    : skiaSurface(std::move(other.skiaSurface)),
      defaultTypeface(std::move(other.defaultTypeface)),
      rt(other.rt), ctx(other.ctx)
{
    other.rt = nullptr;
    other.ctx = nullptr;
}

SingularityGraphics::~SingularityGraphics()
{
    if (ctx) JS_FreeContext(ctx);
    if (rt) JS_FreeRuntime(rt);
    // dmon_deinit();
}

void SingularityGraphics::reloadScript()
{
    if (!scriptDirty) return;
    scriptDirty = false;
    setupJS();
}

// ---- fillStyle ---------------------------------------------------------------
JSValue SingularityGraphics::js_getFillStyle(JSContext* ctx, JSValue, int, JSValue*)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self) return JS_UNDEFINED;
    return JS_NewString(ctx, colorToHex(self->fillStyle).c_str());
}
JSValue SingularityGraphics::js_setFillStyle(JSContext* ctx, JSValue, int argc, JSValue* argv)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self || argc < 1) return JS_UNDEFINED;
    const char* str = JS_ToCString(ctx, argv[0]);
    if (!str) return JS_UNDEFINED;
    self->fillStyle = parseColor(str);
    JS_FreeCString(ctx, str);
    return JS_UNDEFINED;
}

// ---- strokeStyle -------------------------------------------------------------
JSValue SingularityGraphics::js_getStrokeStyle(JSContext* ctx, JSValue, int, JSValue*)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self) return JS_UNDEFINED;
    return JS_NewString(ctx, colorToHex(self->strokeStyle).c_str());
}
JSValue SingularityGraphics::js_setStrokeStyle(JSContext* ctx, JSValue, int argc, JSValue* argv)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self || argc < 1) return JS_UNDEFINED;
    const char* str = JS_ToCString(ctx, argv[0]);
    if (!str) return JS_UNDEFINED;
    self->strokeStyle = parseColor(str);
    JS_FreeCString(ctx, str);
    return JS_UNDEFINED;
}

// ---- lineWidth ---------------------------------------------------------------
JSValue SingularityGraphics::js_getLineWidth(JSContext* ctx, JSValue, int, JSValue*)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self) return JS_UNDEFINED;
    return JS_NewFloat64(ctx, self->lineWidth);
}
JSValue SingularityGraphics::js_setLineWidth(JSContext* ctx, JSValue, int argc, JSValue* argv)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self || argc < 1) return JS_UNDEFINED;
    double v; JS_ToFloat64(ctx, &v, argv[0]);
    self->lineWidth = (float)v;
    return JS_UNDEFINED;
}

// ---- globalAlpha -------------------------------------------------------------
JSValue SingularityGraphics::js_getGlobalAlpha(JSContext* ctx, JSValue, int, JSValue*)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self) return JS_UNDEFINED;
    return JS_NewFloat64(ctx, self->globalAlpha);
}
JSValue SingularityGraphics::js_setGlobalAlpha(JSContext* ctx, JSValue, int argc, JSValue* argv)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self || argc < 1) return JS_UNDEFINED;
    double v; JS_ToFloat64(ctx, &v, argv[0]);
    self->globalAlpha = (float)std::max(0.0, std::min(1.0, v));
    return JS_UNDEFINED;
}

// ---- lineCap -----------------------------------------------------------------
JSValue SingularityGraphics::js_getLineCap(JSContext* ctx, JSValue, int, JSValue*)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self) return JS_UNDEFINED;
    const char* s = "round";
    if (self->lineCap == SkPaint::kButt_Cap)   s = "butt";
    if (self->lineCap == SkPaint::kSquare_Cap)  s = "square";
    return JS_NewString(ctx, s);
}
JSValue SingularityGraphics::js_setLineCap(JSContext* ctx, JSValue, int argc, JSValue* argv)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self || argc < 1) return JS_UNDEFINED;
    const char* str = JS_ToCString(ctx, argv[0]);
    if (!str) return JS_UNDEFINED;
    if (std::string(str) == "butt")        self->lineCap = SkPaint::kButt_Cap;
    else if (std::string(str) == "square") self->lineCap = SkPaint::kSquare_Cap;
    else                                   self->lineCap = SkPaint::kRound_Cap;
    JS_FreeCString(ctx, str);
    return JS_UNDEFINED;
}

// ---- text --------------------------------------------------------------------
JSValue SingularityGraphics::js_fillText(JSContext* ctx, JSValue, int argc, JSValue* argv)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self || argc < 3) return JS_UNDEFINED;
    const char* text = JS_ToCString(ctx, argv[0]);
    if (!text) return JS_UNDEFINED;
    double x, y;
    JS_ToFloat64(ctx, &x, argv[1]);
    JS_ToFloat64(ctx, &y, argv[2]);
    SkFont font(self->defaultTypeface, self->fontSize);
    font.setEdging(SkFont::Edging::kAntiAlias);
    auto paint = self->makeFillPaint();
    self->skiaSurface->getCanvas()->drawString(text, (float)x, (float)y, font, paint);
    JS_FreeCString(ctx, text);
    return JS_UNDEFINED;
}
JSValue SingularityGraphics::js_getFontSize(JSContext* ctx, JSValue, int, JSValue*)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self) return JS_UNDEFINED;
    return JS_NewFloat64(ctx, self->fontSize);
}
JSValue SingularityGraphics::js_setFontSize(JSContext* ctx, JSValue, int argc, JSValue* argv)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self || argc < 1) return JS_UNDEFINED;
    double v; JS_ToFloat64(ctx, &v, argv[0]);
    self->fontSize = (float)v;
    return JS_UNDEFINED;
}

// ---- rectangles --------------------------------------------------------------
JSValue SingularityGraphics::js_fillRect(JSContext* ctx, JSValue, int argc, JSValue* argv)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self || argc < 4) return JS_UNDEFINED;
    double x, y, w, h;
    JS_ToFloat64(ctx, &x, argv[0]); JS_ToFloat64(ctx, &y, argv[1]);
    JS_ToFloat64(ctx, &w, argv[2]); JS_ToFloat64(ctx, &h, argv[3]);
    auto paint = self->makeFillPaint();
    self->skiaSurface->getCanvas()->drawRect(
        SkRect::MakeXYWH((float)x,(float)y,(float)w,(float)h), paint);
    return JS_UNDEFINED;
}
JSValue SingularityGraphics::js_strokeRect(JSContext* ctx, JSValue, int argc, JSValue* argv)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self || argc < 4) return JS_UNDEFINED;
    double x, y, w, h;
    JS_ToFloat64(ctx, &x, argv[0]); JS_ToFloat64(ctx, &y, argv[1]);
    JS_ToFloat64(ctx, &w, argv[2]); JS_ToFloat64(ctx, &h, argv[3]);
    auto paint = self->makeStrokePaint();
    self->skiaSurface->getCanvas()->drawRect(
        SkRect::MakeXYWH((float)x,(float)y,(float)w,(float)h), paint);
    return JS_UNDEFINED;
}
JSValue SingularityGraphics::js_clearRect(JSContext* ctx, JSValue, int argc, JSValue* argv)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self || argc < 4) return JS_UNDEFINED;
    double x, y, w, h;
    JS_ToFloat64(ctx, &x, argv[0]); JS_ToFloat64(ctx, &y, argv[1]);
    JS_ToFloat64(ctx, &w, argv[2]); JS_ToFloat64(ctx, &h, argv[3]);
    SkPaint p; p.setBlendMode(SkBlendMode::kClear);
    self->skiaSurface->getCanvas()->drawRect(
        SkRect::MakeXYWH((float)x,(float)y,(float)w,(float)h), p);
    return JS_UNDEFINED;
}

// ---- paths -------------------------------------------------------------------
JSValue SingularityGraphics::js_beginPath(JSContext* ctx, JSValue, int, JSValue*)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (self) self->currentPath = SkPathBuilder();
    return JS_UNDEFINED;
}
JSValue SingularityGraphics::js_closePath(JSContext* ctx, JSValue, int, JSValue*)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (self) self->currentPath.close();
    return JS_UNDEFINED;
}
JSValue SingularityGraphics::js_moveTo(JSContext* ctx, JSValue, int argc, JSValue* argv)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self || argc < 2) return JS_UNDEFINED;
    double x, y; JS_ToFloat64(ctx, &x, argv[0]); JS_ToFloat64(ctx, &y, argv[1]);
    self->currentPath.moveTo((float)x, (float)y);
    return JS_UNDEFINED;
}
JSValue SingularityGraphics::js_lineTo(JSContext* ctx, JSValue, int argc, JSValue* argv)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self || argc < 2) return JS_UNDEFINED;
    double x, y; JS_ToFloat64(ctx, &x, argv[0]); JS_ToFloat64(ctx, &y, argv[1]);
    self->currentPath.lineTo((float)x, (float)y);
    return JS_UNDEFINED;
}
JSValue SingularityGraphics::js_arc(JSContext* ctx, JSValue, int argc, JSValue* argv)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self || argc < 5) return JS_UNDEFINED;
    double x, y, r, start, end; bool ccw = false;
    JS_ToFloat64(ctx, &x, argv[0]); JS_ToFloat64(ctx, &y, argv[1]);
    JS_ToFloat64(ctx, &r, argv[2]); JS_ToFloat64(ctx, &start, argv[3]);
    JS_ToFloat64(ctx, &end, argv[4]);
    if (argc > 5) ccw = JS_ToBool(ctx, argv[5]);

    float startDeg = (float)(start * 180.0 / std::numbers::pi);
    float sweep    = (float)((end - start) * 180.0 / std::numbers::pi);
    if (!ccw) { while (sweep <= 0.0f) sweep += 360.0f; }  // CW: must be positive
    else      { while (sweep >= 0.0f) sweep -= 360.0f; }  // CCW: must be negative

    SkRect oval = SkRect::MakeLTRB(
        (float)(x-r), (float)(y-r), (float)(x+r), (float)(y+r));
    self->currentPath.addArc(oval, startDeg, sweep);
    return JS_UNDEFINED;
}
JSValue SingularityGraphics::js_bezierCurveTo(JSContext* ctx, JSValue, int argc, JSValue* argv)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self || argc < 6) return JS_UNDEFINED;
    double cp1x,cp1y,cp2x,cp2y,x,y;
    JS_ToFloat64(ctx,&cp1x,argv[0]); JS_ToFloat64(ctx,&cp1y,argv[1]);
    JS_ToFloat64(ctx,&cp2x,argv[2]); JS_ToFloat64(ctx,&cp2y,argv[3]);
    JS_ToFloat64(ctx,&x,   argv[4]); JS_ToFloat64(ctx,&y,   argv[5]);
    self->currentPath.cubicTo(
        (float)cp1x,(float)cp1y,(float)cp2x,(float)cp2y,(float)x,(float)y);
    return JS_UNDEFINED;
}
JSValue SingularityGraphics::js_quadraticCurveTo(JSContext* ctx, JSValue, int argc, JSValue* argv)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self || argc < 4) return JS_UNDEFINED;
    double cpx,cpy,x,y;
    JS_ToFloat64(ctx,&cpx,argv[0]); JS_ToFloat64(ctx,&cpy,argv[1]);
    JS_ToFloat64(ctx,&x,  argv[2]); JS_ToFloat64(ctx,&y,  argv[3]);
    self->currentPath.quadTo((float)cpx,(float)cpy,(float)x,(float)y);
    return JS_UNDEFINED;
}
JSValue SingularityGraphics::js_fill(JSContext* ctx, JSValue, int, JSValue*)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self) return JS_UNDEFINED;
    auto paint = self->makeFillPaint();
    self->skiaSurface->getCanvas()->drawPath(self->currentPath.snapshot(), paint);
    return JS_UNDEFINED;
}
JSValue SingularityGraphics::js_stroke(JSContext* ctx, JSValue, int, JSValue*)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self) return JS_UNDEFINED;
    auto paint = self->makeStrokePaint();
    self->skiaSurface->getCanvas()->drawPath(self->currentPath.snapshot(), paint);
    return JS_UNDEFINED;
}

// ---- transforms -------------------------------------------------------------
JSValue SingularityGraphics::js_save(JSContext* ctx, JSValue, int, JSValue*)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self) return JS_UNDEFINED;
    self->stateStack.push_back({
        self->fillStyle, self->strokeStyle,
        self->lineWidth, self->globalAlpha,
        self->lineCap,   self->lineJoin
    });
    self->skiaSurface->getCanvas()->save();
    return JS_UNDEFINED;
}
JSValue SingularityGraphics::js_restore(JSContext* ctx, JSValue, int, JSValue*)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self || self->stateStack.empty()) return JS_UNDEFINED;
    auto& s = self->stateStack.back();
    self->fillStyle   = s.fillStyle;
    self->strokeStyle = s.strokeStyle;
    self->lineWidth   = s.lineWidth;
    self->globalAlpha = s.globalAlpha;
    self->lineCap     = s.lineCap;
    self->lineJoin    = s.lineJoin;
    self->stateStack.pop_back();
    self->skiaSurface->getCanvas()->restore();
    return JS_UNDEFINED;
}
JSValue SingularityGraphics::js_translate(JSContext* ctx, JSValue, int argc, JSValue* argv)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self || argc < 2) return JS_UNDEFINED;
    double x, y; JS_ToFloat64(ctx, &x, argv[0]); JS_ToFloat64(ctx, &y, argv[1]);
    self->skiaSurface->getCanvas()->translate((float)x, (float)y);
    return JS_UNDEFINED;
}
JSValue SingularityGraphics::js_rotate(JSContext* ctx, JSValue, int argc, JSValue* argv)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self || argc < 1) return JS_UNDEFINED;
    double angle; JS_ToFloat64(ctx, &angle, argv[0]);
    self->skiaSurface->getCanvas()->rotate((float)(angle * 180.0 / std::numbers::pi));
    return JS_UNDEFINED;
}
JSValue SingularityGraphics::js_scale(JSContext* ctx, JSValue, int argc, JSValue* argv)
{
    auto* self = (SingularityGraphics*)JS_GetContextOpaque(ctx);
    if (!self || argc < 2) return JS_UNDEFINED;
    double x, y; JS_ToFloat64(ctx, &x, argv[0]); JS_ToFloat64(ctx, &y, argv[1]);
    self->skiaSurface->getCanvas()->scale((float)x, (float)y);
    return JS_UNDEFINED;
}

const void* SingularityGraphics::getPixels() const
{
    SkPixmap pixmap;
    skiaSurface->peekPixels(&pixmap);
    return pixmap.addr();
}

int SingularityGraphics::getWidth() const     { return skiaSurface->width(); }
int SingularityGraphics::getHeight() const    { return skiaSurface->height(); }

size_t SingularityGraphics::getRowBytes() const
{
    SkPixmap pixmap;
    skiaSurface->peekPixels(&pixmap);
    return pixmap.rowBytes();
}

JSValue SingularityGraphics::js_hello(JSContext* ctx, JSValue, int, JSValue*)
{
    std::cout << "Hello from JavaScript!" << std::endl;
    return JS_UNDEFINED;
}
