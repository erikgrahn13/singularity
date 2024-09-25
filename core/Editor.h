#ifndef EDITOR_H
#define EDITOR_H

#include "include/core/SkCanvas.h" // Skia included in C++ code only
#include "include/core/SkFontStyle.h"
#include "include/core/SkSurface.h"
#include "include/private/base/SkFeatures.h"

#if defined(SK_BUILD_FOR_WIN) && (defined(SK_FONTMGR_GDI_AVAILABLE) || defined(SK_FONTMGR_DIRECTWRITE_AVAILABLE))
#include "include/ports/SkTypeface_win.h"
#endif
#if defined(SK_BUILD_FOR_UNIX)
#include "include/ports/SkFontMgr_fontconfig.h"
#endif

#if (defined(SK_BUILD_FOR_IOS) || defined(SK_BUILD_FOR_MAC))
#include "include/ports/SkFontMgr_mac_ct.h"
#endif

class Editor
{
  public:
    Editor(int width, int height);
    virtual ~Editor();
    void draw(SkCanvas *canvas);
    void loadFont();

    sk_sp<SkSurface> skiaSurface;

    const char *tostr(SkFontStyle::Slant s)
    {
        switch (s)
        {
        case SkFontStyle::kUpright_Slant:
            return "SkFontStyle::kUpright_Slant";
        case SkFontStyle::kItalic_Slant:
            return "SkFontStyle::kItalic_Slant";
        case SkFontStyle::kOblique_Slant:
            return "SkFontStyle::kOblique_Slant";
        default:
            return "";
        }
    }

  protected:
    // virtual void initializeSkiaSurface() = 0;
    sk_sp<SkFontMgr> fontMgr;

    int width;
    int height;
};

#endif
