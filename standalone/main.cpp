#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

#include <iostream>
#include "../SingularityGraphics2.h"

// static SDL_Window *window = NULL;
// static SDL_Renderer *renderer = NULL;

struct AppState 
{
    SDL_Window *window{nullptr};
    SDL_Renderer *sdlRenderer{nullptr};
    SDL_Texture *texture{nullptr};
    std::unique_ptr<SingularityGraphics> graphics;
} ;

/* This function runs once at startup. */
SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
{
//  AppState *state = (AppState*)SDL_calloc(1, sizeof(AppState));
    auto state = std::make_unique<AppState>();
    state->window = SDL_CreateWindow("Hello World", 800, 600, 0);  // no SDL_WINDOW_FULLSCREEN
    if (!state->window) {
        SDL_Log("Couldn't create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->sdlRenderer = SDL_CreateRenderer(state->window, nullptr);
    if (!state->sdlRenderer) {
        SDL_Log("Couldn't create renderer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    // XRGB8888: memory layout [B,G,R,X] on LE — matches Skia N32/BGRA, alpha ignored
    state->texture = SDL_CreateTexture(state->sdlRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 800, 600);
    if (!state->texture) {
        SDL_Log("Couldn't create texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state->graphics = std::make_unique<SingularityGraphics>(800, 600);

    *appstate = state.release();

    SDL_Log("Hej App");

    return SDL_APP_CONTINUE;
}

/* This function runs when a new event (mouse input, keypresses, etc) occurs. */
SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    if ( event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;  /* end the program, reporting success to the OS. */
    }
    return SDL_APP_CONTINUE;
}

/* This function runs once per frame, and is the heart of the program. */
SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState* state = static_cast<AppState*>(appstate);
    DrawingContent dc = state->graphics->getRenderData();

    if (!dc.contentAddres) {
        SDL_Log("getRenderData returned null pixel pointer");
        return SDL_APP_CONTINUE;
    }

    // Upload Skia pixels to SDL texture (contentBytes is rowBytes/pitch)
    if (!SDL_UpdateTexture(state->texture, nullptr, dc.contentAddres, (int)dc.contentBytes)) {
        SDL_Log("SDL_UpdateTexture failed: %s", SDL_GetError());
    }

    SDL_RenderClear(state->sdlRenderer);
    SDL_RenderTexture(state->sdlRenderer, state->texture, nullptr, nullptr);
    SDL_RenderPresent(state->sdlRenderer);
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