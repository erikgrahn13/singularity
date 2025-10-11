#ifndef EDITORWIN_H
#define EDITORWIN_H

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include "Editor.h"

class EditorWin
{
  public:
    EditorWin(int width, int height);
    // void initializeSkiaSurface() override;
    void draw(SkCanvas *canvas)
    {
        mEditor->draw(canvas);
    }

    sk_sp<SkSurface> skiaSurface;
    BITMAPINFO bmi;

  private:
    std::unique_ptr<Editor> mEditor;
};

#endif