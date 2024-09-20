#ifndef EDITOR_H
#define EDITOR_H

#include "include/core/SkCanvas.h" // Include SkCanvas
#include "include/core/SkSurface.h"

class Editor
{
  public:
    Editor(int width, int height);
    virtual ~Editor();

    void draw(SkCanvas *canvas);

    sk_sp<SkSurface> skiaSurface;
    int width;
    int height;

  protected:
    virtual void initializeSkiaSurface() = 0;
};

#endif
