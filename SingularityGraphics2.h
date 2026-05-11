#pragma once

#if __has_include(<swift/bridging>)
#  include <swift/bridging>
#else
#  define SWIFT_RETURNS_INDEPENDENT_VALUE
#endif

#include <memory>
#include <atomic>
#include <functional>
#include <unordered_map>
// Include interface headers - they're lightweight (just virtual functions)
#include "IRenderer.h"
#include "IJSEngine.h"
#include "IFileWatcher.h"
#include "IParameterProvider.h"

namespace visage { class Canvas; }


class SingularityGraphics {


    public:
    SingularityGraphics(int width, int height, IParameterProvider &parameterProvider,
                        bool standalone = false,
                        const std::string& scriptPath = {});

    SWIFT_RETURNS_INDEPENDENT_VALUE
    DrawingContent getRenderData();

    std::vector<uint8_t> encodeFrameToPng() { return renderer->encodeFrameToPng(); }

    void hotReload();
    void loadScript(const std::string& path);

    void addParameter();

    void setOnOpenSettings(std::function<void()> cb);
    void setOnSetBloom(std::function<void(float, float)> cb);
    void setStringList(const std::string& key, std::vector<std::string> values);

    // Events
    void onMouseDown(float x, float y);
    void onMouseUp(float x, float y);
    void onMouseMove(float x, float y);

    // tmp
    void renderUI(visage::Canvas& canvas);
    void renderFrame(float t);
    // void setParameter(int id, double value) override { parameterProvider. parameters[id] = value; }
    double getParameter(int id) {
        return parameterProvider.getParameter(id);
    }



    std::atomic<bool> pendingReload { false };

    private:
    std::unique_ptr<IRenderer> renderer;
    std::unique_ptr<IJSEngine> jsEngine;
    std::unique_ptr<IFileWatcher> fileWatcher;
    IParameterProvider& parameterProvider;
    std::function<void()> onOpenSettings;

    //TODO: tmp
    // std::unordered_map<int, double> parameters;
};
