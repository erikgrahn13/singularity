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

void SkiaRenderer::clear()
{
    skiaSurface->getCanvas()->clear(SK_ColorBLACK);
}

// TODO: implement all the names colors, and also HSLA and RGBA
void SkiaRenderer::setFillStyle(const std::string& color)
{
    if (color.empty()) 
    {
        fillStyle = SK_ColorBLACK;
        return;
    }

    if (color[0] == '#') {
        std::string hex = color.substr(1);
        if (hex.size() == 3)
            hex = { hex[0], hex[0], hex[1], hex[1], hex[2], hex[2] };
        if (hex.size() == 6) {
            unsigned int rgb = (unsigned int)std::stoul(hex, nullptr, 16);
            fillStyle = SkColorSetARGB(0xFF, (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF, rgb & 0xFF);
            return;
        } else if (hex.size() == 8) {
            unsigned long rgba = std::stoul(hex, nullptr, 16);
            fillStyle = SkColorSetARGB(rgba & 0xFF, (rgba >> 24) & 0xFF, (rgba >> 16) & 0xFF, (rgba >> 8) & 0xFF);
            return;
        }
        fillStyle = SK_ColorBLACK;
        return;
    }
}


void SkiaRenderer::fillRect(float x, float y, float width, float height)
{
    SkPaint paint;
    paint.setColor(fillStyle);
    skiaSurface->getCanvas()->drawRect(SkRect::MakeXYWH(x, y, width, height), paint);
}

void SkiaRenderer::beginPath()
{
    currentPath = SkPathBuilder();
}

void SkiaRenderer::stroke()
{
    SkPaint paint;
    paint.setColor(fillStyle);
    skiaSurface->getCanvas()->drawPath(currentPath.snapshot(), paint);
}


void SkiaRenderer::arc(float x, float y, float radius, float startAngle, float endAngle)
{
    // SkPaint paint;
    // paint.setColor(fillStyle);
    // currentPath.addArc(SkRect::MakeXYWH)
    // skiaSurface->getCanvas()->drawArc(SkArc::Make(SkRect::MakeXYWH(x, y, radius, radius), SkScalar(startAngle), SkScalar(endAngle), SkArc::Type::kArc), paint);
}



std::unique_ptr<IRenderer> createRenderer(int width, int height)
{
    return std::make_unique<SkiaRenderer>(width, height);
}