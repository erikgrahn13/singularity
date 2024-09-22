#include <SDL.h>
#include <SDL_main.h>

int main(int argc, char *argv[])
{
    SDL_Init(SDL_INIT_VIDEO);

    SDL_Window *screen =
        SDL_CreateWindow("My SDL Empty Window", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, 0);

    // Event loop to keep the window open
    bool running = true; // Flag to indicate whether the window is open
    SDL_Event event;     // Variable to store the event

    while (running)
    {
        // Poll for events (non-blocking)
        while (SDL_PollEvent(&event) != 0)
        {
            // Handle window close event
            if (event.type == SDL_QUIT)
            {
                running = false; // Exit the loop when the window close button is clicked
            }
        }
        // Additional rendering or processing code can go here if needed
    }

    // Clean up and quit SDL
    SDL_DestroyWindow(screen);
    SDL_Quit();

    return 0;
}