#include "Editor.h"

#include "include/core/SkCanvas.h" // Include SkCanvas
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkGraphics.h"
#include "include/core/SkPaint.h" // Include SkPaint
#include "include/core/SkSurface.h"
#include "include/core/SkTextBlob.h"

Editor::Editor(int width, int height)
{
    SkGraphics::Init();
}

Editor::~Editor()
{
}

void Editor::draw(SkCanvas *canvas)
{
    // Get the canvas from the surface
    int height = canvas->getBaseLayerSize().height();
    int width = canvas->getBaseLayerSize().width();

    //     // Define Skia paint object
    SkPaint paint;

    // Use Skia predefined colors to test
    paint.setColor(SK_ColorRED);
    canvas->drawRect(SkRect::MakeXYWH(0, 0, 100, 100), paint); // Should draw red

    paint.setColor(SK_ColorGREEN);
    canvas->drawRect(SkRect::MakeXYWH(100, 0, 100, 100), paint); // Should draw green

    paint.setColor(SK_ColorBLUE);
    canvas->drawRect(SkRect::MakeXYWH(200, 0, 100, 100), paint); // Should draw green
}
