#include "chocQuickJSEngine.h"
#include <print>
#include <format>

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
    auto wrapped = "(() => {\n" + code + "\n})();";

    ctx.run(wrapped, [](const std::string& error, const choc::value::ValueView&) {
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

    ctx.registerFunction("__stroke", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->stroke();
        return {};
    });

    ctx.registerFunction("__save", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->save();
        return {};
    });

    ctx.registerFunction("__restore", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->restore();
        return {};
    });

    // Properties
    ctx.registerFunction("__setFillStyle", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->setFillStyle(std::string(args[0]->getString()));
        return {};
    });

    ctx.registerFunction("__setGlobalAlpha", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->setGlobalAlpha(args[0]->getFloat64());
        return {};
    });

    ctx.registerFunction("__setStrokeStyle", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->setStrokeStyle(std::string(args[0]->getString()));
        return {};
    });

    ctx.registerFunction("__setLineWidth", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->setLineWidth(args[0]->getFloat64());
        return {};
    });

    ctx.registerFunction("__setLineCap", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->setLineCap(std::string(args[0]->getString()));
        return {};
    });

    ctx.registerFunction("__setLineJoin", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->setLineJoin(std::string(args[0]->getString()));
        return {};
    });

    ctx.registerFunction("__strokeRect", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->strokeRect(args[0]->getFloat64(), args[1]->getFloat64(),
                           args[2]->getFloat64(), args[3]->getFloat64());
        return {};
    });

    ctx.registerFunction("__roundRect", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->roundRect(args[0]->getFloat64(), args[1]->getFloat64(),
                             args[2]->getFloat64(), args[3]->getFloat64(),
                             args[4]->getFloat64());
        return {};
    });

    ctx.registerFunction("__fill", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->fill();
        return {};
    });

    ctx.registerFunction("__moveTo", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->moveTo(args[0]->getFloat64(), args[1]->getFloat64());
        return {};
    });

    auto bootstrap = std::format(R"(
        const _ctx = (() => {{
            let _fillStyle = '#000000';
            let _globalAlpha = 1.0;
            let _strokeStyle = '#000000';
            let _lineWidth = 1.0;
            let _lineCap = 'butt';
            let _lineJoin = 'miter';  
            
            const obj = {{
                fillRect: (x, y, w, h) => __fillRect(x, y, w, h),
                arc: (x, y, radius, startAngle, endAngle) => __arc(x, y, radius, startAngle, endAngle),
                beginPath: () =>  __beginPath(),
                stroke: () => __stroke(),
                save: () => __save(),
                restore: () => __restore(),
                strokeRect: (x, y, w, h) => __strokeRect(x, y, w, h),
                roundRect: (x, y, w, h, radii) => __roundRect(x, y, w, h, radii),
                fill: () => __fill(),
                moveTo: (x, y) => __moveTo(x, y),
            }};
            Object.defineProperty(obj, 'fillStyle', {{
                get: ()  => _fillStyle,
                set: (v) => {{ _fillStyle = v; __setFillStyle(v); }},
            }});
            Object.defineProperty(obj, 'globalAlpha', {{
                get: ()  => _globalAlpha,
                set: (v) => {{ _globalAlpha = v; __setGlobalAlpha(v); }},
            }});
            Object.defineProperty(obj, 'strokeStyle', {{
                get: ()  => _strokeStyle,
                set: (v) => {{ _strokeStyle = v; __setStrokeStyle(v); }},
            }});
            Object.defineProperty(obj, 'lineWidth', {{
                get: ()  => _lineWidth,
                set: (v) => {{ _lineWidth = v; __setLineWidth(v); }},
            }});
            Object.defineProperty(obj, 'lineCap', {{
                get: ()  => _lineCap,
                set: (v) => {{ _lineCap = v; __setLineCap(v); }},
            }});
            Object.defineProperty(obj, 'lineJoin', {{
                get: ()  => _lineJoin,
                set: (v) => {{ _lineJoin = v; __setLineJoin(v); }},
            }});
            return obj;
        }})();

        const _canvas = {{
            width: {},
            height: {},
            getContext: (type) => _ctx,
        }};

        const document = {{
            getElementById: (id) => _canvas,
        }};
    )", renderer->getWidth(), renderer->getHeight());

    ctx.run(bootstrap);
}

