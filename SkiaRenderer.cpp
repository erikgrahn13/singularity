#include "SkiaRenderer.h"
#include "include/core/SkCanvas.h"

SkiaRenderer::SkiaRenderer()
{
    skiaSurface = SkSurfaces::Raster(SkImageInfo::MakeN32Premul(300, 200));
    SkCanvas* canvas = skiaSurface->getCanvas();
    canvas->drawColor(SK_ColorGREEN);

}

DrawingContent SkiaRenderer::getDrawingContent()
{
    SkPixmap pixmap;
    skiaSurface->peekPixels(&pixmap);
    return {pixmap.addr(), pixmap.rowBytes(), skiaSurface->width(), skiaSurface->height()};
}

std::unique_ptr<IRenderer> createRenderer()
{
    return std::make_unique<SkiaRenderer>();
}