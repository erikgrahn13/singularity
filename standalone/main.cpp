#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

#include <iostream>
#include "../SingularityGraphics2.h"

struct AppState 
{
    SDL_Window *window{nullptr};
    std::unique_ptr<SingularityGraphics> graphics;
} ;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
    auto state = std::make_unique<AppState>();
    state->window = SDL_CreateWindow("Hello World", 800, 600, 0); 
    if (!state->window) {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    auto sdlSurface = SDL_GetWindowSurface(state->window);
    state->graphics = std::make_unique<SingularityGraphics>(sdlSurface->w, sdlSurface->h);

    *appstate = state.release();

    SDL_Log("Hej App");

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState* state = static_cast<AppState*>(appstate);
    
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        // SDL_Log("Mouse clicked x: %f   y: %f", event->button.x, event->button.y);
        state->graphics->onMouseDown(event->button.x, event->button.y);
        
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        break;
    case SDL_EVENT_MOUSE_MOTION:
        break;
    case SDL_EVENT_MOUSE_WHEEL:
        break;
    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState* state = static_cast<AppState*>(appstate);

    //     float t = SDL_GetTicks() / 1000.0f;
    // state->graphics->renderFrame(t);
#if !defined NDEBUG
    if(state->graphics->pendingReload.exchange(false))
    {
        state->graphics->hotReload();
    }
#endif



    DrawingContent dc = state->graphics->getRenderData();

    if (!dc.contentAddres) {
        SDL_Log("getRenderData returned null pixel pointer");
        return SDL_APP_CONTINUE;
    }

    auto sdlSurface = SDL_GetWindowSurface(state->window);
    auto skiaSurface =  SDL_CreateSurfaceFrom(dc.width, dc.height, sdlSurface->format, (void*)dc.contentAddres, dc.contentBytes);

    SDL_BlitSurface(skiaSurface, nullptr, sdlSurface, nullptr);


    SDL_UpdateWindowSurface(state->window);
    SDL_DestroySurface(skiaSurface);

    return SDL_APP_CONTINUE;
}

/* This function runs once at shutdown. */
void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    std::unique_ptr<AppState> state(static_cast<AppState*>(appstate));
    // destructor cleans up graphics automatically
    // manually destroy window before AppState destructs:
    if (state && state->window) {
        SDL_DestroyWindow(state->window);
    }
}