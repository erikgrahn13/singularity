#include "ExampleEditor.h"
#include "include/core/SkFont.h"

ExampleEditor::ExampleEditor(int width, int height) : Editor(width, height)
{
}

void ExampleEditor::draw(SkCanvas *canvas)
{
    // Get the canvas from the surface
    int height = canvas->getBaseLayerSize().height();
    int width = canvas->getBaseLayerSize().width();

    //     // Define Skia paint object
    SkPaint paint;
    canvas->clear(SK_ColorWHITE);

    // Use Skia predefined colors to test
    paint.setColor(SK_ColorRED);
    canvas->drawRect(SkRect::MakeXYWH(0, 0, 100, 100), paint); // Should draw red

    paint.setColor(SK_ColorGREEN);
    canvas->drawRect(SkRect::MakeXYWH(100, 0, 100, 100), paint); // Should draw green

    paint.setColor(SK_ColorBLUE);
    canvas->drawRect(SkRect::MakeXYWH(200, 0, 100, 100), paint); // Should draw green

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
}
