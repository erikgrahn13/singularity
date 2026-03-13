#include "SingularityGraphics.h"
#include "include/core/SkStream.h"
#include "include/core/SkData.h"
#include "include/core/SkImage.h"
#include "include/core/SkRRect.h"

#ifdef _WIN64
#include <windows.h>
#endif


SingularityGraphics::SingularityGraphics()
{
    SkImageInfo info = SkImageInfo::MakeN32Premul(800, 600); // Set width/height accordingly
    skiaSurface = SkSurfaces::Raster(info);

    SkCanvas* canvas = skiaSurface->getCanvas();
    canvas->drawColor(SK_ColorGREEN);
}

void SingularityGraphics::DrawRectangle()
{
    SkCanvas *canvas = skiaSurface->getCanvas();
    SkRect rect = SkRect::MakeXYWH(10, 10, 100, 160);

    SkPaint paint;
    paint.setStyle(SkPaint::kFill_Style);
    paint.setAntiAlias(true);
    paint.setStrokeWidth(4);
    paint.setColor(SK_ColorYELLOW);

    canvas->drawRect(rect, paint);
}