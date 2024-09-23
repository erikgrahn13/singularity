#include <SDL.h>
#include <SDL_main.h>
#include <iostream>

// Skia includes
#include "include/core/SkCanvas.h"
#include "include/core/SkImageInfo.h"
#include "include/core/SkPixmap.h"
#include "include/core/SkSurface.h"

int main(int argc, char *argv[])
{
    // Initialize SDL's video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        std::cerr << "SDL could not initialize! SDL_Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create an SDL window without SDL_WINDOW_SOFTWARE flag (which doesn't exist in SDL2)
    SDL_Window *window = SDL_CreateWindow("SDL + Skia CPU Rendering", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          640, 480, SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window)
    {
        std::cerr << "Window could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create an SDL renderer
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer)
    {
        std::cerr << "Renderer could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create an SDL texture to render the Skia output
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 640, 480);
    if (!texture)
    {
        std::cerr << "Texture could not be created! SDL_Error: " << SDL_GetError() << std::endl;
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Create an offscreen Skia surface for CPU rendering
    SkImageInfo info = SkImageInfo::Make(640, 480, kBGRA_8888_SkColorType, kPremul_SkAlphaType);
    sk_sp<SkSurface> skSurface = SkSurfaces::Raster(info);

    if (!skSurface)
    {
        std::cerr << "Failed to create Skia surface!" << std::endl;
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
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

        // Flush the Skia canvas to make sure all drawing is completed
        // canvas->flush();

        // Get a pointer to the texture pixel buffer
        void *pixels = nullptr;
        int pitch = 0;
        SDL_LockTexture(texture, NULL, &pixels, &pitch);

        // Copy Skia's pixel data to the SDL texture
        SkPixmap pixmap;
        if (skSurface->peekPixels(&pixmap))
        {
            memcpy(pixels, pixmap.addr(), pixmap.computeByteSize());
        }

        SDL_UnlockTexture(texture);

        // Clear the renderer and copy the texture to the screen
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, texture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    // Clean up
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
