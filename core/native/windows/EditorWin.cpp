#include "EditorWin.h"

EditorWin::EditorWin(int width, int height)
{
    // Create a Skia surface that matches the window dimensions
    SkImageInfo info = SkImageInfo::MakeN32Premul(800, 600); // Set width/height accordingly
    skiaSurface = SkSurfaces::Raster(info);

    // Initialize GDI bitmap info
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = info.width();
    bmi.bmiHeader.biHeight = -info.height(); // Negative for top-down bitmap
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    mEditor = createEditorInstance();
}

// void EditorWin::initializeSkiaSurface()
// {
//     // Create a Skia surface that matches the window dimensions
//     SkImageInfo info = SkImageInfo::MakeN32Premul(800, 600); // Set width/height accordingly
//     skiaSurface = SkSurfaces::Raster(info);

//     // Initialize GDI bitmap info
//     memset(&bmi, 0, sizeof(bmi));
//     bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
//     bmi.bmiHeader.biWidth = info.width();
//     bmi.bmiHeader.biHeight = -info.height(); // Negative for top-down bitmap
//     bmi.bmiHeader.biPlanes = 1;
//     bmi.bmiHeader.biBitCount = 32;
//     bmi.bmiHeader.biCompression = BI_RGB;
// }
