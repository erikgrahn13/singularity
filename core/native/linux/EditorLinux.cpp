#include "EditorLinux.h"

EditorLinux::EditorLinux(int width, int height) : Editor(width, height)
{
    SkImageInfo imageInfo = SkImageInfo::MakeN32Premul(width, height);
    skiaSurface = SkSurfaces::Raster(imageInfo);
}