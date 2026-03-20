#include "chocQuickJSEngine.h"
#include <print>

#include "choc/text/choc_Files.h"

std::unique_ptr<IJSEngine> createJSEngine()
{
    return std::make_unique<ChocQuickJSEngine>();
}

ChocQuickJSEngine::ChocQuickJSEngine()
 : ctx(choc::javascript::createQuickJSContext())
{

}

void ChocQuickJSEngine::hotReload()
{
    std::println("HOT RELOAD");
    auto code = choc::file::loadFileAsString(JS_SCRIPTS_DIR"/hello.js");

    ctx.run(code, [](const std::string& error, const choc::value::ValueView&) {
    if (!error.empty())
        std::println("JS error: {}", error);
    });
}

void ChocQuickJSEngine::bindRenderer(IRenderer *renderer)
{
    ctx.registerFunction("__fillRect", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->fillRect(args[0]->getFloat64(), args[1]->getFloat64(),
                           args[2]->getFloat64(), args[3]->getFloat64());
        return {};
    });

    ctx.registerFunction("__arc", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->arc(args[0]->getFloat64(), args[1]->getFloat64(),
                           args[2]->getFloat64(), args[3]->getFloat64(),
                           args[4]->getFloat64());
        return {};
    });

    ctx.registerFunction("__beginPath", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->beginPath();
        return {};
    });

    ctx.registerFunction("__setFillStyle", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->setFillStyle(std::string(args[0]->getString()));
        return {};
    });

    ctx.run(R"(
        const ctx = (() => {
            let _fillStyle = '#000000';
            const obj = {
                fillRect: (x, y, w, h) => __fillRect(x, y, w, h),
                arc: (x, y, radius, startAngle, endAngle) => __arc(x, y, radius, startAngle, endAngle),
                beginPath: __beginPath()
            };
            Object.defineProperty(obj, 'fillStyle', {
                get: ()  => _fillStyle,
                set: (v) => { _fillStyle = v; __setFillStyle(v); },
            });
            return obj;
        })();
    )");
}

