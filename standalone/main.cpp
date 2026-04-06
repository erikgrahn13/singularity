#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

#include <iostream>
#include <limits>
#include "../SingularityGraphics2.h"
#include "IParameterProvider.h"
#include "ISingularityAudio.h"

 // TODO: should be real time safe with real time safe queue
class ParameterContainer : public IParameterProvider {

    public:
    double getParameter(int id) const override {
        auto it = params.find(id);
        if (it != params.end()) return it->second.value;
        return std::numeric_limits<double>::quiet_NaN(); // Unknown parameter
    }
    void setParameter(int id, double value) override {
        auto it = params.find(id);
        if (it != params.end()) it->second.value = value; // Only update existing parameters
    }
    std::unordered_map<int, Parameter> params;
};

struct AppState 
{
    SDL_Window *window{nullptr};
    SDL_Window *settingsWindow{nullptr};
    std::unique_ptr<SingularityGraphics> controller;
    std::unique_ptr<ISingularityAudio> processor;
    ParameterContainer parameters;
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

    state->parameters.params[13] = { .name = "Volume", .type = ParamType::Float, .value = 0.1, .minValue = 0.0, .maxValue = 1.0, .defaultValue = 0.5 };
    state->parameters.params[7]  = { .name = "Bypass",   .type = ParamType::Bool,    .value = 0.0, .defaultValue = 0.0 };
    state->parameters.params[15] = { .name = "Waveform", .type = ParamType::Stepped, .value = 0.0, .defaultValue = 0.0, .steps = 4 };

    auto sdlSurface = SDL_GetWindowSurface(state->window);
    state->controller = std::make_unique<SingularityGraphics>(sdlSurface->w, sdlSurface->h, state->parameters);
    state->controller->setOnOpenSettings([state = state.get()]() {
        state->settingsWindow = SDL_CreateWindow("Settings", 400, 300, SDL_WINDOW_ALWAYS_ON_TOP);
        SDL_SetWindowParent(state->settingsWindow, state->window);
        SDL_SetWindowModal(state->settingsWindow, true);
        SDL_RaiseWindow(state->settingsWindow);
    });
    state->processor = ISingularityAudio::createSingularityAudio();
    state->processor->probeDevices();

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
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        if (state->settingsWindow && event->window.windowID == SDL_GetWindowID(state->settingsWindow)) {
            SDL_DestroyWindow(state->settingsWindow);
            state->settingsWindow = nullptr;
        } else {
            return SDL_APP_SUCCESS;
        }
        break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
        // SDL_Log("Mouse clicked x: %f   y: %f", event->button.x, event->button.y);
        state->controller->onMouseDown(event->button.x, event->button.y);
    
        break;
    case SDL_EVENT_MOUSE_BUTTON_UP:
        state->controller->onMouseUp(event->button.x, event->button.y);
        break;
    case SDL_EVENT_MOUSE_MOTION:
        state->controller->onMouseMove(event->button.x, event->button.y);
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
    if(state->controller->pendingReload.exchange(false))
    {
        state->controller->hotReload();
    }
#endif


    state->controller->renderUI();
    DrawingContent dc = state->controller->getRenderData();

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