#include "include/core/SkCanvas.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPaint.h"
#include "include/core/SkSurface.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <iostream>

int main()
{
    Display *display;
    Window window;
    XEvent event;
    int screen;

    // Open connection to the X server
    display = XOpenDisplay(NULL);
    if (display == NULL)
    {
        std::cerr << "Cannot open display\n";
        return 1;
    }

    screen = DefaultScreen(display);

    // Create an X11 window
    window = XCreateSimpleWindow(display, RootWindow(display, screen), 10, 10, 800, 600, 1, BlackPixel(display, screen),
                                 WhitePixel(display, screen));
    XSelectInput(display, window, ExposureMask | KeyPressMask);
    XMapWindow(display, window);

    // Create Skia surface for CPU rendering
    SkImageInfo imageInfo = SkImageInfo::MakeN32Premul(800, 600);
    auto surface = SkSurfaces::Raster(imageInfo);

    SkCanvas *canvas = surface->getCanvas();

    // Skia drawing (e.g., red circle)
    SkPaint paint;
    paint.setColor(SK_ColorRED);
    canvas->clear(SK_ColorWHITE);
    canvas->drawCircle(400, 300, 100, paint);

    // Event loop
    while (true)
    {
        XNextEvent(display, &event);

        if (event.type == Expose)
        {
            // Skia has already rendered onto its surface, now we copy the pixel data to X11

            // Get the raw pixel buffer from Skia
            SkPixmap pixmap;
            if (surface->peekPixels(&pixmap))
            {
                // Convert Skia pixels into XImage format for X11
                XImage *ximage = XCreateImage(display, DefaultVisual(display, screen), 24, ZPixmap, 0,
                                              (char *)pixmap.addr(), 800, 600, 32, pixmap.rowBytes());
                // Transfer the image to the X11 window
                XPutImage(display, window, DefaultGC(display, screen), ximage, 0, 0, 0, 0, 800, 600);
                XFlush(display);
                ximage->data = nullptr; // To prevent X11 from freeing Skia's pixel data
                XDestroyImage(ximage);  // Clean up XImage struct, but keep Skia surface intact
            }
        }

        // Close on keypress
        if (event.type == KeyPress)
        {
            break;
        }
    }

    // Cleanup
    XCloseDisplay(display);
    return 0;
}
