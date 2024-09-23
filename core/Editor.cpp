#include "Editor.h"

#include "include/core/SkCanvas.h" // Include SkCanvas
#include "include/core/SkFont.h"
#include "include/core/SkFontMetrics.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkGraphics.h"
#include "include/core/SkPaint.h" // Include SkPaint
#include "include/core/SkSurface.h"
#include "include/core/SkTextBlob.h"
#include "tools/fonts/FontToolUtils.h"

Editor::Editor(int width, int height)
{
    SkGraphics::Init();

#if defined(SK_BUILD_FOR_WIN) && (defined(SK_FONTMGR_GDI_AVAILABLE) || defined(SK_FONTMGR_DIRECTWRITE_AVAILABLE))
#include "include/ports/SkTypeface_win.h"
#endif

#if defined(SK_BUILD_FOR_ANDROID) && defined(SK_FONTMGR_ANDROID_AVAILABLE)
#include "include/ports/SkFontMgr_android.h"
#include "src/ports/SkTypeface_FreeType.h"
#endif

#if (defined(SK_BUILD_FOR_IOS) || defined(SK_BUILD_FOR_MAC))
    fontMgr = SkFontMgr_New_CoreText(nullptr);
#endif
}

Editor::~Editor()
{
}

void Editor::loadFont()
{
    // load the desired font here
    // sk_sp<SkTypeface> typeface = fontMgr->makeFromFile(fontPath);
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

    // sk_sp<SkTypeface> face = ToolUtils::CreatePortableTypeface("Helvetica", SkFontStyle());

    // // // sk_sp<SkFontMgr> mgr = SkFontMgr::RefEmpty();
    // // sk_sp<SkTypeface> face = mgr->matchFamilyStyle("Helvetica", SkFontStyle());
    // // // sk_sp<SkFontMgr> fontMgr = SkFontMgr::RefEmpty();

    // // // Load the font from a file (e.g., .ttf or .otf)

    // if (!face)
    // {
    //     printf("Cannot open typeface\n");
    //     return 1;
    // }

    // SkFont font(face, 40);

    // const char *text = "HELLO SINGULARITY";

    // // // Measure the width of the text
    // SkScalar textWidth = font.measureText(text, strlen(text), SkTextEncoding::kUTF8);
    // // Get the font metrics to measure the height of the text
    // SkFontMetrics metrics;
    // font.getMetrics(&metrics);

    // // // The text height is the distance between the top and bottom of the text
    // SkScalar textHeight = metrics.fCapHeight; // You can use metrics.capHeight() for precise height

    // // Calculate the x and y positions to center the text
    // SkScalar x = (width - textWidth) / 2;
    // SkScalar y = (height + textHeight) / 2;

    // paint.setColor(SK_ColorGREEN);
    // canvas->drawString(text, x, y, font, paint);

    float x = 10, y = 10;
    float textScale = 24;

    for (int i = 0; i < fontMgr->countFamilies(); ++i)
    {
        SkString familyName;
        fontMgr->getFamilyName(i, &familyName);
        sk_sp<SkFontStyleSet> styleSet(fontMgr->createStyleSet(i));
        for (int j = 0; j < styleSet->count(); ++j)
        {
            SkFontStyle fontStyle;
            SkString style;
            styleSet->getStyle(j, &fontStyle, &style);
            auto s = SkStringPrintf("SkFont font(fontMgr->legacyMakeTypeface(\"%s\", SkFontStyle(%3d, %1d, %-27s), "
                                    "%g);",
                                    familyName.c_str(), fontStyle.weight(), fontStyle.width(), tostr(fontStyle.slant()),
                                    textScale);
            SkFont font(fontMgr->legacyMakeTypeface(familyName.c_str(), fontStyle), textScale);
            y += font.getSpacing() * 1.5;
            canvas->drawString(s, x, y, font, SkPaint());
        }
    }

    // test
}
