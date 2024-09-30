#ifndef EDITORMAC_H
#define EDITORMAC_H

#include "Editor.h"
#include <CoreGraphics/CoreGraphics.h>

class EditorMac
{
  public:
    EditorMac();
    // void initializeSkiaSurface() override;
    void draw(CGContextRef context);

  private:
    std::unique_ptr<Editor> mEditor;
};

#endif // EDITORMAC_H