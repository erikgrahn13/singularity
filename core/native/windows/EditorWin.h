#ifndef EDITORWIN_H
#define EDITORWIN_H

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include "Editor.h"

class EditorWin : public Editor
{
  public:
    EditorWin(int width, int height);
    void initializeSkiaSurface() override;

    BITMAPINFO bmi;
};

#endif