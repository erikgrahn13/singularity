#include "SkiaRenderer.h"
#include "include/core/SkCanvas.h"

SkiaRenderer::SkiaRenderer(int width, int height)
{
    skiaSurface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(width, height));

    SkCanvas* canvas = skiaSurface->getCanvas();
    canvas->drawColor(SK_ColorGREEN);

}

DrawingContent SkiaRenderer::getDrawingContent()
{
    SkPixmap pixmap;
    skiaSurface->peekPixels(&pixmap);
    return {pixmap.addr(), pixmap.rowBytes(), skiaSurface->width(), skiaSurface->height()};
}

SkColor SkiaRenderer::applyAlpha(SkColor color) const
{
    uint8_t a = (uint8_t)(SkColorGetA(color) * currentState.globalAlpha);
    return SkColorSetA(color, a);
}

void SkiaRenderer::clear()
{
    skiaSurface->getCanvas()->clear(SK_ColorBLACK);
    currentState = DrawState{};
    stateStack.clear();
}

void SkiaRenderer::save()
{
    stateStack.push_back(currentState);
    skiaSurface->getCanvas()->save();
}

void SkiaRenderer::restore()
{
    if(stateStack.empty())
        return;
    currentState = stateStack.back();
    stateStack.pop_back();
    skiaSurface->getCanvas()->restore();
}

void SkiaRenderer::setGlobalAlpha(float alpha)
{
    currentState.globalAlpha = alpha;
}

// TODO: implement all the names colors, and also HSLA and RGBA
void SkiaRenderer::setFillStyle(const std::string& color)
{
    if (color.empty()) 
    {
        currentState.fillStyle = SK_ColorBLACK;
        return;
    }

    if (color[0] == '#') {
        std::string hex = color.substr(1);
        if (hex.size() == 3)
            hex = { hex[0], hex[0], hex[1], hex[1], hex[2], hex[2] };
        if (hex.size() == 6) {
            unsigned int rgb = (unsigned int)std::stoul(hex, nullptr, 16);
            currentState.fillStyle = SkColorSetARGB(0xFF, (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
            return;
        } else if (hex.size() == 8) {
            unsigned long rgba = std::stoul(hex, nullptr, 16);
            currentState.fillStyle = SkColorSetARGB(rgba & 0xFF, (rgba >> 24) & 0xFF, (rgba >> 16) & 0xFF, (rgba >> 8) & 0xFF);
            return;
        }
        currentState.fillStyle = SK_ColorBLACK;
    }
}

void SkiaRenderer::setStrokeStyle(const std::string &color)
{
    if (color.empty()) 
    {
        currentState.strokeStyle = SK_ColorBLACK;
        return;
    }

    if (color[0] == '#') {
        std::string hex = color.substr(1);
        if (hex.size() == 3)
            hex = { hex[0], hex[0], hex[1], hex[1], hex[2], hex[2] };
        if (hex.size() == 6) {
            unsigned int rgb = (unsigned int)std::stoul(hex, nullptr, 16);
            currentState.strokeStyle = SkColorSetARGB(0xFF, (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
            return;
        } else if (hex.size() == 8) {
            unsigned long rgba = std::stoul(hex, nullptr, 16);
            currentState.strokeStyle = SkColorSetARGB(rgba & 0xFF, (rgba >> 24) & 0xFF, (rgba >> 16) & 0xFF, (rgba >> 8) & 0xFF);
            return;
        }
        currentState.strokeStyle = SK_ColorBLACK;
    }
}

void SkiaRenderer::setLineWidth(float lineWidth)
{
    currentState.lineWidth = lineWidth;
}

void SkiaRenderer::setLineCap(const std::string &cap)
{
    if(cap == "butt")
    {
        currentState.lineCap = SkPaint::kButt_Cap;
    }
    else if(cap == "round")
    {
        currentState.lineCap = SkPaint::kRound_Cap;
    }
    else if(cap == "square")
    {
        currentState.lineCap = SkPaint::kSquare_Cap;
    }
}

void SkiaRenderer::fillRect(float x, float y, float width, float height)
{
    SkPaint paint;
    paint.setColor(applyAlpha(currentState.fillStyle));
    skiaSurface->getCanvas()->drawRect(SkRect::MakeXYWH(x, y, width, height), paint);
}

void SkiaRenderer::beginPath()
{
    currentPath = SkPathBuilder();
}

void SkiaRenderer::stroke()
{
    SkPaint paint;
    paint.setStyle(SkPaint::kStroke_Style);
    paint.setColor(applyAlpha(currentState.strokeStyle));
    paint.setStrokeWidth(currentState.lineWidth);
    paint.setStrokeCap(currentState.lineCap);
    paint.setStrokeJoin(currentState.lineJoin);
    skiaSurface->getCanvas()->drawPath(currentPath.snapshot(), paint);
}


void SkiaRenderer::arc(float x, float y, float radius, float startAngle, float endAngle)
{
    SkRect oval = SkRect::MakeXYWH(x - radius, y - radius, radius * 2, radius *2);
    float sweepDeg = (endAngle - startAngle) * (180.f / SK_FloatPI);
    currentPath.addArc(oval, startAngle * (180.f / SK_FloatPI), sweepDeg);
}



std::unique_ptr<IRenderer> createRenderer(int width, int height)
{
    return std::make_unique<SkiaRenderer>(width, height);
}