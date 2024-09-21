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

    sk_sp<SkSurface> skiaSurface;

  protected:
    // virtual void initializeSkiaSurface() = 0;

    int width;
    int height;
};

#endif
