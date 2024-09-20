#include "EditorMac.h"
#include "include/core/SkGraphics.h"
#include "include/core/SkSurface.h"
#include "include/core/SkPaint.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColorSpace.h"
#include <iostream>
#include <CoreFoundation/CoreFoundation.h>

void setupSkia()
{
    SkGraphics::Init();
}

void drawSkia(CGContextRef context)
{
    // Log the CGContext width and height
    size_t width = CGBitmapContextGetWidth(context);
    size_t height = CGBitmapContextGetHeight(context);
    size_t rowBytes = CGBitmapContextGetBytesPerRow(context);

    CGContextClearRect(context, CGRectMake(0, 0, width, height));

    // Set up Skia's image info with BGRA color type
    SkImageInfo imageInfo = SkImageInfo::Make(
        width,
        height,
        kBGRA_8888_SkColorType,  // Explicitly use BGRA color type
        kPremul_SkAlphaType,
        SkColorSpace::MakeSRGB()
    );

    // Get the pixel buffer from the CGContext
    void* pixels = CGBitmapContextGetData(context);

    // Wrap Skia surface around the CGContext's pixel buffer
    sk_sp<SkSurface> surface = SkSurfaces::WrapPixels(imageInfo, pixels, rowBytes);

    SkCanvas* canvas = surface->getCanvas();

    // Clear the canvas to white
    canvas->clear(SK_ColorWHITE);

    // Define Skia paint object
    SkPaint paint;

    // Use Skia predefined colors to test
    paint.setColor(SK_ColorRED);
    canvas->drawRect(SkRect::MakeXYWH(0, 0, 100, 100), paint);  // Should draw red

    paint.setColor(SK_ColorGREEN);
    canvas->drawRect(SkRect::MakeXYWH(100, 0, 100, 100), paint);  // Should draw green

    paint.setColor(SK_ColorBLUE);
    canvas->drawRect(SkRect::MakeXYWH(200, 0, 100, 100), paint);  // Should draw green

}