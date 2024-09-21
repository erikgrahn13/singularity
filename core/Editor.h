#ifndef EDITOR_H
#define EDITOR_H

#include "include/core/SkCanvas.h" // Skia included in C++ code only
#include "include/core/SkSurface.h"

class Editor
{
  public:
    Editor(int width, int height);
    virtual ~Editor();
    void draw(SkCanvas *canvas);

  protected:
    // virtual void initializeSkiaSurface() = 0;

    sk_sp<SkSurface> skiaSurface;
    int width;
    int height;
};

#endif
