#pragma once

#include <windows.h>
#include "include/core/SkPixmap.h"
#include "SingularityGraphics.h"

class SingularityGraphicsWin {
public:
    static void Blit(HDC hdc, SingularityGraphics& graphics) {
        SkPixmap pixmap;
        if (!graphics.skiaSurface->peekPixels(&pixmap))
            return;

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth       = pixmap.width();
        bmi.bmiHeader.biHeight      = -pixmap.height(); // top-down
        bmi.bmiHeader.biPlanes      = 1;
        bmi.bmiHeader.biBitCount    = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        SetDIBitsToDevice(hdc,
            0, 0, pixmap.width(), pixmap.height(),
            0, 0, 0, pixmap.height(),
            pixmap.addr(), &bmi, DIB_RGB_COLORS);
    }
};