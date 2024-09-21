#include "Editor.h"

#include "include/core/SkCanvas.h" // Include SkCanvas
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkGraphics.h"
#include "include/core/SkPaint.h" // Include SkPaint
#include "include/core/SkSurface.h"

#include "include/core/SkTextBlob.h"
#include "include/ports/SkFontMgr_fontconfig.h"

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
    // SkCanvas *canvas = editor->skiaSurface->getCanvas();
    int height = canvas->getBaseLayerSize().height();
    int width = canvas->getBaseLayerSize().width();

    //     const char *fontFamily = "Helvetica"; // Default system family
    //     SkFontStyle fontStyle;                // Default weight, width, slant

    //     sk_sp<SkTypeface> typeface = fontMgr->legacyMakeTypeface(fontFamily, fontStyle);
    //     SkPaint paint;
    //     paint.setColor(SK_ColorRED);
    //     SkFont font;
    //     // font.setTypeface();
    //     font.setSize(20);
    //     const char *str = "Hello Singularity";
    //     canvas->drawSimpleText(str, strlen(str), SkTextEncoding::kUTF8, 100, 100, font, paint);

    SkPaint paint;
    paint.setColor(SkColors::kRed);
    const char *test = "Hello Erik";
    SkFont font;
    canvas->drawSimpleText(test, strlen(test), SkTextEncoding::kUTF8, 100, 100, )
}

// Draw something with Skia
// SkPaint paint;
// paint.setColor(SK_ColorRED);
// canvas->drawRect(SkRect::MakeXYWH(100, 100, 200, 200), paint); // Draw red rectangle
// }
