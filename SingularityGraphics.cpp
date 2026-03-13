#include "SingularityGraphics.h"
#include <iostream>

SingularityGraphics::SingularityGraphics()
    : SingularityGraphics(JS_SCRIPTS_DIR "/hello.js")
{}

SingularityGraphics::SingularityGraphics(const std::string& jsPath)
{
    SkImageInfo info = SkImageInfo::MakeN32Premul(300, 200);
    skiaSurface = SkSurfaces::Raster(info);

    SkCanvas* canvas = skiaSurface->getCanvas();
    canvas->drawColor(SK_ColorGREEN);

    rt = JS_NewRuntime();
    ctx = JS_NewContext(rt);

    js_std_add_helpers(ctx, 0, nullptr);
    js_init_module_std(ctx, "std");

    JSValue global = JS_GetGlobalObject(ctx);

    JS_SetPropertyStr(ctx, global, "hello",
    JS_NewCFunction(ctx, js_hello, "hello", 0));

    JS_FreeValue(ctx, global);

    if (!jsPath.empty()) {
        size_t buf_len = 0;
        uint8_t* buf = js_load_file(ctx, &buf_len, jsPath.c_str());
        if (buf) {
            JSValue result = JS_Eval(ctx, (const char*)buf, buf_len, jsPath.c_str(), JS_EVAL_TYPE_GLOBAL);
            if (JS_IsException(result)) {
                js_std_dump_error(ctx);
            }
            JS_FreeValue(ctx, result);
            js_free(ctx, buf);
        } else {
            std::cerr << "[SingularityGraphics] Could not load JS file: " << jsPath << std::endl;
        }
    }
}

SingularityGraphics::SingularityGraphics(SingularityGraphics&& other) noexcept
    : skiaSurface(std::move(other.skiaSurface)),
      rt(other.rt), ctx(other.ctx)
{
    other.rt = nullptr;
    other.ctx = nullptr;
}

SingularityGraphics::~SingularityGraphics()
{
    if (ctx) JS_FreeContext(ctx);
    if (rt) JS_FreeRuntime(rt);
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
