// #define SDL_MAIN_USE_CALLBACKS 1
// #include <SDL3/SDL_main.h>
// #include <SDL3/SDL.h>

// #include <iostream>
// #include <limits>
// #include "../SingularityGraphics2.h"
// #include "IParameterProvider.h"
// #include "ISingularityAudio.h"

//  // TODO: should be real time safe with real time safe queue
// class ParameterContainer : public IParameterProvider {

//     public:
//     double getParameter(int id) const override {
//         auto it = params.find(id);
//         if (it != params.end()) return it->second.value;
//         return std::numeric_limits<double>::quiet_NaN(); // Unknown parameter
//     }
//     void setParameter(int id, double value) override {
//         auto it = params.find(id);
//         if (it != params.end()) it->second.value = value; // Only update existing parameters
//     }
//     std::unordered_map<int, Parameter> params;
// };

// struct AppState 
// {
//     SDL_Window *window{nullptr};
//     SDL_Window *settingsWindow{nullptr};
//     std::unique_ptr<SingularityGraphics> controller;
//     std::unique_ptr<SingularityGraphics> settingsController;
//     std::unique_ptr<ISingularityAudio> processor;
//     ParameterContainer parameters;
// } ;

// /* This function runs once at startup. */
// SDL_AppResult SDL_AppInit(void **appstate, int argc, char *argv[])
// {
//     auto state = std::make_unique<AppState>();
//     state->window = SDL_CreateWindow("Hello World", 800, 600, 0); 
//     if (!state->window) {
//         SDL_Log("Couldn't create window: %s", SDL_GetError());
//         return SDL_APP_FAILURE;
//     }

//     state->parameters.params[13] = { .name = "Volume", .type = ParamType::Float, .value = 0.1, .minValue = 0.0, .maxValue = 1.0, .defaultValue = 0.5 };
//     state->parameters.params[7]  = { .name = "Bypass",   .type = ParamType::Bool,    .value = 0.0, .defaultValue = 0.0 };
//     state->parameters.params[15] = { .name = "Waveform", .type = ParamType::Stepped, .value = 0.0, .defaultValue = 0.0, .steps = 4 };

//     auto sdlSurface = SDL_GetWindowSurface(state->window);
//     state->controller = std::make_unique<SingularityGraphics>(sdlSurface->w, sdlSurface->h, state->parameters);
//     state->controller->loadScript(JS_SCRIPTS_DIR"/hello.js");
//     state->controller->setOnOpenSettings([state = state.get()]() {
//         if (state->settingsWindow) return; // already open

//         // Build string lists before constructing so they are ready when loadScript runs
//         auto devices = state->processor->probeDevices();
//         std::unordered_map<std::string, std::vector<std::string>> stringLists;
//         stringLists["audioBackends"] = ISingularityAudio::backends;
//         for (const auto& backend : ISingularityAudio::backends) {
//             std::vector<std::string> deviceNames;
//             for (const auto& d : devices)
//                 deviceNames.push_back(d.name);
//             stringLists["devices:" + backend] = std::move(deviceNames);
//         }

//         state->settingsWindow = SDL_CreateWindow("Settings", 400, 300, 0);
//         SDL_SetWindowParent(state->settingsWindow, state->window);
//         SDL_SetWindowModal(state->settingsWindow, true);
//         SDL_RaiseWindow(state->settingsWindow);
//         state->settingsController = std::make_unique<SingularityGraphics>(400, 300, state->parameters);
//         for (auto& [key, values] : stringLists)
//             state->settingsController->setStringList(key, values);
//         state->settingsController->loadScript(JS_SCRIPTS_DIR"/widgets/settings.js");
//     });
//     state->processor = ISingularityAudio::createSingularityAudio();

//     *appstate = state.release();

//     SDL_Log("Hej App");

//     return SDL_APP_CONTINUE;
// }

// /* This function runs when a new event (mouse input, keypresses, etc) occurs. */
// SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
// {
//     AppState* state = static_cast<AppState*>(appstate);
    
//     switch (event->type)
//     {
//     case SDL_EVENT_QUIT:
//         return SDL_APP_SUCCESS;
//         break;
//     case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
//         if (state->settingsWindow && event->window.windowID == SDL_GetWindowID(state->settingsWindow)) {
//             SDL_DestroyWindow(state->settingsWindow);
//             state->settingsWindow = nullptr;
//             state->settingsController.reset();
//         } else {
//             return SDL_APP_SUCCESS;
//         }
//         break;
//     case SDL_EVENT_MOUSE_BUTTON_DOWN:
//         if (state->settingsWindow && event->window.windowID == SDL_GetWindowID(state->settingsWindow))
//             state->settingsController->onMouseDown(event->button.x, event->button.y);
//         else
//             state->controller->onMouseDown(event->button.x, event->button.y);
//         break;
//     case SDL_EVENT_MOUSE_BUTTON_UP:
//         if (state->settingsWindow && event->window.windowID == SDL_GetWindowID(state->settingsWindow))
//             state->settingsController->onMouseUp(event->button.x, event->button.y);
//         else
//             state->controller->onMouseUp(event->button.x, event->button.y);
//         break;
//     case SDL_EVENT_MOUSE_MOTION:
//         if (state->settingsWindow && event->window.windowID == SDL_GetWindowID(state->settingsWindow))
//             state->settingsController->onMouseMove(event->button.x, event->button.y);
//         else
//             state->controller->onMouseMove(event->button.x, event->button.y);
//         break;
//     case SDL_EVENT_MOUSE_WHEEL:
//         break;
//     default:
//         break;
//     }
//     return SDL_APP_CONTINUE;
// }

// /* This function runs once per frame, and is the heart of the program. */
// SDL_AppResult SDL_AppIterate(void *appstate)
// {
//     AppState* state = static_cast<AppState*>(appstate);

//     //     float t = SDL_GetTicks() / 1000.0f;
//     // state->graphics->renderFrame(t);
// #if !defined NDEBUG
//     if(state->controller->pendingReload.exchange(false))
//     {
//         state->controller->hotReload();
//     }
// #endif


//     state->controller->renderUI();
//     DrawingContent dc = state->controller->getRenderData();

//     if (!dc.contentAddres) {
//         SDL_Log("getRenderData returned null pixel pointer");
//         return SDL_APP_CONTINUE;
//     }

//     auto sdlSurface = SDL_GetWindowSurface(state->window);
//     auto skiaSurface =  SDL_CreateSurfaceFrom(dc.width, dc.height, sdlSurface->format, (void*)dc.contentAddres, dc.contentBytes);

//     SDL_BlitSurface(skiaSurface, nullptr, sdlSurface, nullptr);


//     SDL_UpdateWindowSurface(state->window);
//     SDL_DestroySurface(skiaSurface);

//     if (state->settingsWindow && state->settingsController) {
// #if !defined NDEBUG
//         if (state->settingsController->pendingReload.exchange(false))
//             state->settingsController->hotReload();
// #endif
//         state->settingsController->renderUI();
//         DrawingContent sdc = state->settingsController->getRenderData();
//         if (sdc.contentAddres) {
//             auto settingsSurface = SDL_GetWindowSurface(state->settingsWindow);
//             auto skiaSettingsSurface = SDL_CreateSurfaceFrom(sdc.width, sdc.height, settingsSurface->format, (void*)sdc.contentAddres, sdc.contentBytes);
//             SDL_BlitSurface(skiaSettingsSurface, nullptr, settingsSurface, nullptr);
//             SDL_UpdateWindowSurface(state->settingsWindow);
//             SDL_DestroySurface(skiaSettingsSurface);
//         }
//     }

//     return SDL_APP_CONTINUE;
// }

// /* This function runs once at shutdown. */
// void SDL_AppQuit(void *appstate, SDL_AppResult result)
// {
//     std::unique_ptr<AppState> state(static_cast<AppState*>(appstate));
//     // destructor cleans up graphics automatically
//     // manually destroy window before AppState destructs:
//     if (state && state->window) {
//         SDL_DestroyWindow(state->window);
//     }
// }

#include "NativeWindow.h"
#include "../SingularityGraphics2.h"
#include "../IParameterProvider.h"

#include <limits>
#include <memory>
#include <unordered_map>

// ---------------------------------------------------------------------------
// Simple parameter store for the standalone app
// ---------------------------------------------------------------------------
class ParameterContainer : public IParameterProvider {
public:
    double getParameter(int id) const override {
        auto it = params.find(id);
        if (it != params.end()) return it->second.value;
        return std::numeric_limits<double>::quiet_NaN();
    }
    void setParameter(int id, double value) override {
        auto it = params.find(id);
        if (it != params.end()) it->second.value = value;
    }
    std::unordered_map<int, Parameter> params;
};

// ---------------------------------------------------------------------------
int main()
{
    ParameterContainer params;
    params.params[13] = { .name = "Volume",   .type = ParamType::Float,   .value = 0.1, .minValue = 0.0, .maxValue = 1.0, .defaultValue = 0.5 };
    params.params[7]  = { .name = "Bypass",   .type = ParamType::Bool,    .value = 0.0, .defaultValue = 0.0 };
    params.params[15] = { .name = "Waveform", .type = ParamType::Stepped, .value = 0.0, .defaultValue = 0.0, .steps = 4 };

    auto graphics = std::make_unique<SingularityGraphics>(PLUGIN_WIDTH, PLUGIN_HEIGHT, params, /*standalone=*/true);

    auto win = createNativeWindow("Singularity", PLUGIN_WIDTH, PLUGIN_HEIGHT);

    // Settings window — created on demand, destroyed on close
    std::unique_ptr<IWindow> settingsWin;
    std::unique_ptr<SingularityGraphics> settingsGraphics;
    bool settingsDirty = false;

    graphics->setOnOpenSettings([&]() {
        if (settingsWin) return; // already open

        settingsGraphics = std::make_unique<SingularityGraphics>(400, 300, params, true, std::string(JS_SCRIPTS_DIR) + "/widgets/settings.js");

        settingsWin = createNativeWindow("Settings", 400, 300, nullptr, win.get());
        settingsDirty = true;

        settingsWin->setOnMouseDown([&](int x, int y, unsigned int) { settingsGraphics->onMouseDown((float)x, (float)y); settingsDirty = true; });
        settingsWin->setOnMouseUp  ([&](int x, int y, unsigned int) { settingsGraphics->onMouseUp  ((float)x, (float)y); settingsDirty = true; });
        settingsWin->setOnMouseMove([&](int x, int y)               { settingsGraphics->onMouseMove((float)x, (float)y); settingsDirty = true; });
        settingsWin->setOnFrame([&]() -> DrawingContent {
            if (!settingsDirty) return {};
            settingsGraphics->renderUI();
            settingsDirty = false;
            return settingsGraphics->getRenderData();
        });

        settingsWin->setOnClose([&]() {
            settingsGraphics.reset();
            settingsWin.reset();
        });
    });

    win->setOnMouseDown([&](int x, int y, unsigned int /*btn*/) { graphics->onMouseDown((float)x, (float)y); });
    win->setOnMouseUp  ([&](int x, int y, unsigned int /*btn*/) { graphics->onMouseUp  ((float)x, (float)y); });
    win->setOnMouseMove([&](int x, int y)                       { graphics->onMouseMove((float)x, (float)y); });

    win->setOnFrame([&]() -> DrawingContent {
#ifndef NDEBUG
        if (graphics->pendingReload.exchange(false)) {
            graphics->hotReload();
        }
#endif
        graphics->renderUI();
        return graphics->getRenderData();
    });

    win->run();
}

