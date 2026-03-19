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

std::unique_ptr<IRenderer> createRenderer(int width, int height)
{
    return std::make_unique<SkiaRenderer>(width, height);
}