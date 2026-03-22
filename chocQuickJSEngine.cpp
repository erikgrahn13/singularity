#include "chocQuickJSEngine.h"
#include <print>
#include <format>

#include "choc/text/choc_Files.h"
#include "choc/javascript/choc_javascript_Console.h"

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
    choc::javascript::registerConsoleFunctions(ctx);

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

    ctx.registerFunction("__lineTo", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->lineTo(args[0]->getFloat64(), args[1]->getFloat64());
        return {};
    });

    ctx.registerFunction("__closePath", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->closePath();
        return {};
    });
    
    ctx.registerFunction("__quadraticCurveTo", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->quadraticCurveTo(args[0]->getFloat64(), args[1]->getFloat64(),
                                   args[2]->getFloat64(), args[3]->getFloat64());
        return {};
    });

    ctx.registerFunction("__bezierCurveTo", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->bezierCurveTo(args[0]->getFloat64(), args[1]->getFloat64(),
                                args[2]->getFloat64(), args[3]->getFloat64(),
                                args[4]->getFloat64(), args[5]->getFloat64());
        return {};
    });

    ctx.registerFunction("__arcTo", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->arcTo(args[0]->getFloat64(), args[1]->getFloat64(),
                                args[2]->getFloat64(), args[3]->getFloat64(),
                                args[4]->getFloat64());
        return {};
    });

    ctx.registerFunction("__ellipse", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->ellipse(args[0]->getFloat64(), args[1]->getFloat64(),
                          args[2]->getFloat64(), args[3]->getFloat64(),
                          args[4]->getFloat64(), args[5]->getFloat64(),
                          args[6]->getFloat64());
        return {};
    });

    ctx.registerFunction("__rect", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->rect(args[0]->getFloat64(), args[1]->getFloat64(),
                          args[2]->getFloat64(), args[3]->getFloat64());
        return {};
    });

    ctx.registerFunction("__fillText", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->fillText(std::string(args[0]->getString()), args[1]->getFloat64(),
                           args[2]->getFloat64());
        return {};
    });

    ctx.registerFunction("__strokeText", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->strokeText(std::string(args[0]->getString()), args[1]->getFloat64(),
                           args[2]->getFloat64());
        return {};
    });

    ctx.registerFunction("__measureText", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        auto width = renderer->measureText(std::string(args[0]->getString()));
        return choc::value::Value(width);
    });

    ctx.registerFunction("__setFont", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->font(std::string(args[0]->getString()));
        return {};
    });

    ctx.registerFunction("__textAlign", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->textAlign(std::string(args[0]->getString()));
        return {};
    });

    ctx.registerFunction("__textBaseline", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->textBaseline(std::string(args[0]->getString()));
        return {};
    });

    ctx.registerFunction("__translate", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->translate(args[0]->getFloat64(), args[1]->getFloat64());
        return {};
    });

    ctx.registerFunction("__rotate", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->rotate(args[0]->getFloat64());
        return {};
    });

    ctx.registerFunction("__scale", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->scale(args[0]->getFloat64(), args[1]->getFloat64());
        return {};
    });

    ctx.registerFunction("__resetTransform", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->resetTransform();
        return {};
    });

    ctx.registerFunction("__setShadowColor", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->setShadowColor(std::string(args[0]->getString()));
        return {};
    });

    ctx.registerFunction("__setShadowBlur", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->setShadowBlur(args[0]->getFloat64());
        return {};
    });

    ctx.registerFunction("__setShadowOffsetX", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->setShadowOffsetX(args[0]->getFloat64());
        return {};
    });

    ctx.registerFunction("__setShadowOffsetY", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->setShadowOffsetY(args[0]->getFloat64());
        return {};
    });

    ctx.registerFunction("__createLinearGradient", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        auto gradient = renderer->createLinearGradient(args[0]->getFloat64(), args[1]->getFloat64(),
                                       args[2]->getFloat64(), args[3]->getFloat64());
        return choc::value::Value(gradient);
    });

    ctx.registerFunction("__createRadialGradient", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        auto gradient = renderer->createRadialGradient(args[0]->getFloat64(), args[1]->getFloat64(),
                                                       args[2]->getFloat64(), args[3]->getFloat64(),
                                                       args[4]->getFloat64(), args[5]->getFloat64());
        return choc::value::Value(gradient);
    });

    ctx.registerFunction("__addColorStop", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->addColorStop(args[0]->getFloat64(), args[1]->getFloat64(),
                               std::string(args[2]->getString()));
        return {};
    });

    ctx.registerFunction("__setFillStyleGradient", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->setFillStyleGradient(args[0]->getFloat64());
        return {};
    });

    ctx.registerFunction("__drawImage", [renderer](choc::javascript::ArgumentList args) -> choc::value::Value
    {
        renderer->drawImage(std::string(args[0]->getString()), args[1]->getFloat64(),
                            args[2]->getFloat64(), args[3]->getFloat64(), args[4]->getFloat64());
        return {};
    });

    auto bootstrap = std::format(R"(
        const _addColorStop = __addColorStop;
        __addColorStop = undefined;

        const _ctx = (() => {{
            let _fillStyle = '#000000';
            let _globalAlpha = 1.0;
            let _strokeStyle = '#000000';
            let _lineWidth = 1.0;
            let _lineCap = 'butt';
            let _lineJoin = 'miter'; 
            let _font = '10px sans-serif';
            let _textAlign = 'start';
            let _textBaseline = 'alphabetic';
            let _shadowColor = 'transparent';
            let _shadowBlur = 0.0;
            let _shadowOffsetX = 0.0;
            let _shadowOffsetY = 0.0;
            
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
                lineTo: (x, y) => __lineTo(x, y),
                closePath: () => __closePath(),
                quadraticCurveTo: (cpx, cpy, x, y) => __quadraticCurveTo(cpx, cpy, x, y),
                bezierCurveTo: (cp1x, cp1y, cp2x, cp2y, x, y) => __bezierCurveTo(cp1x, cp1y, cp2x, cp2y, x, y),
                arcTo: (x1, y1, x2, y2, radius) => __arcTo(x1, y1, x2, y2, radius),
                ellipse: (x, y, radiusX, radiusY, rotation, startAngle, endAngle) => __ellipse(x, y, radiusX, radiusY, rotation, startAngle, endAngle),
                rect: (x, y, width, height) => __rect(x, y, width, height),
                fillText: (text, x, y) => __fillText(text, x, y),
                strokeText: (text, x, y) => __strokeText(text, x, y),
                measureText: (text) => ({{ width: __measureText(text) }}),
                translate: (x, y) => __translate(x, y),
                rotate: (angle) => __rotate(angle),
                scale: (x, y) => __scale(x, y),
                resetTransform: () => __resetTransform(),
                drawImage: (name, dx, dy, dw, dh) => __drawImage(name, dx, dy, dw, dh),
                createLinearGradient: (x0, y0, x1, y1) => {{
                    const id = __createLinearGradient(x0, y0, x1, y1);
                    return {{
                        _id: id,
                        addColorStop: (offset, color) => _addColorStop(id, offset, color)
                    }};
                }},
                createRadialGradient: (x0, y0, r0, x1, y1, r1) => {{
                    const id = __createRadialGradient(x0, y0, r0, x1, y1, r1);
                    return {{
                        _id: id,
                        addColorStop: (offset, color) => _addColorStop(id, offset, color)
                    }};
                }},
            }};
            Object.defineProperty(obj, 'fillStyle', {{
                get: ()  => _fillStyle,
                set: (v) => {{ _fillStyle = v; if (typeof v === 'object' && v._id !== undefined) __setFillStyleGradient(v._id); else __setFillStyle(v); }},
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
            Object.defineProperty(obj, 'font', {{
                get: ()  => _font,
                set: (v) => {{ _font = v; __setFont(v); }},
            }});
            Object.defineProperty(obj, 'textAlign', {{
                get: ()  => _textAlign,
                set: (v) => {{ _textAlign = v; __textAlign(v); }},
            }});
            Object.defineProperty(obj, 'textBaseline', {{
                get: ()  => _textBaseline,
                set: (v) => {{ _textBaseline = v; __textBaseline(v); }},
            }});
            Object.defineProperty(obj, 'shadowColor', {{
                get: ()  => _shadowColor,
                set: (v) => {{ _shadowColor = v; __setShadowColor(v); }},
            }});
            Object.defineProperty(obj, 'shadowBlur', {{
                get: ()  => _shadowBlur,
                set: (v) => {{ _shadowBlur = v; __setShadowBlur(v); }},
            }});
            Object.defineProperty(obj, 'shadowOffsetX', {{
                get: ()  => _shadowOffsetX,
                set: (v) => {{ _shadowOffsetX = v; __setShadowOffsetX(v); }},
            }});
            Object.defineProperty(obj, 'shadowOffsetY', {{
                get: ()  => _shadowOffsetY,
                set: (v) => {{ _shadowOffsetY = v; __setShadowOffsetY(v); }},
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

