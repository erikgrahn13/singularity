#include <SDL.h>
#include <SDL_main.h>
#include <iostream>

// Skia includes
#include "include/core/SkCanvas.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkSurface.h"
#include "include/core/SkSurfaceProps.h"

int main(int argc, char *argv[])
{
    // Initialize SDL's video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create an SDL window
    SDL_Window *window = SDL_CreateWindow("SDL + Skia CPU Rendering", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          640, 480, SDL_WINDOW_SHOWN);
    if (!window)
    {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Get the SDL window surface (this is a CPU-based surface)
    SDL_Surface *windowSurface = SDL_GetWindowSurface(window);

    // Create an Skia ImageInfo that matches the SDL surface format
    SkImageInfo info =
        SkImageInfo::Make(windowSurface->w, windowSurface->h, kBGRA_8888_SkColorType, kPremul_SkAlphaType);

    // Wrap the SDL surface's pixel buffer using Skia's WrapPixels function
    sk_sp<SkSurface> skSurface = SkSurfaces::WrapPixels(info, windowSurface->pixels, windowSurface->pitch);

    if (!skSurface)
    {
        std::cerr << "Failed to create Skia surface!" << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Event loop to keep the window open
    bool running = true;
    SDL_Event event;

    while (running)
    {
        // Poll for events (non-blocking)
        while (SDL_PollEvent(&event) != 0)
        {
            // Handle window close event
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }

        // Get the Skia canvas and start drawing
        SkCanvas *canvas = skSurface->getCanvas();

        // Clear the canvas
        canvas->clear(SK_ColorWHITE);

        // Draw a red rectangle using Skia
        SkPaint paint;
        paint.setColor(SK_ColorRED);
        canvas->drawRect(SkRect::MakeXYWH(100, 100, 200, 200), paint);

        // Make sure the Skia drawing is flushed to the surface
        // canvas->flush();

        // Update the SDL window surface with the rendered content
        SDL_UpdateWindowSurface(window);
    }

    // Clean up
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
