// EditorFactory.cpp
#include "EditorFactory.h"
#include "EditorMac.h" // Include EditorMac in the .cpp file, hidden from Swift

extern "C" void *createEditorMac()
{
    return new EditorMac(); // Return as void*
}

extern "C" void destroyEditorMac(void *editor)
{
    delete static_cast<EditorMac *>(editor); // Cast back to EditorMac* and delete
}

extern "C" void drawEditorMac(void *editor, CGContextRef context)
{
    static_cast<EditorMac *>(editor)->draw(context); // Cast back to EditorMac* and call draw()
}