// EditorFactory.h
#ifndef EDITOR_FACTORY_H
#define EDITOR_FACTORY_H

#include <CoreGraphics/CoreGraphics.h>

// Return an opaque pointer instead of the actual EditorMac*
extern "C" void *createEditorMac(int width, int height);
extern "C" void destroyEditorMac(void *editor);
extern "C" void drawEditorMac(void *editor, CGContextRef context);

#endif // EDITOR_FACTORY_H