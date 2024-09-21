#ifndef EDITORMAC_H
#define EDITORMAC_H

#include "Editor.h"
#include <CoreGraphics/CoreGraphics.h>

class EditorMac : public Editor
{
  public:
    EditorMac(int width, int height);
    // void initializeSkiaSurface() override;
    void draw(CGContextRef context);
};

#endif // EDITORMAC_H