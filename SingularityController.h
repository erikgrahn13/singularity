#pragma once
#include "IRenderer2.h"
#include "IJSEngine.h"
#include "IFileWatcher.h"
#include "IParameterProvider.h"
#include <atomic>

class SingularityController {
    public:
    SingularityController(void* rootFrame, IParameterProvider &parameterProvider);
    void initialize();
    void tick(); // call from main thread each frame
    void setLogger(IJSEngine::LogCallback cb);

    void* getRootFrame()
    {
        return renderer_->getRootComponent();
    }

    private:
    void reload();

    std::unique_ptr<IRenderer> renderer_;
    std::unique_ptr<IJSEngine> jsEngine_;
    std::unique_ptr<IFileWatcher> fileWatcher_;
    std::atomic<bool> reloadPending_{false};
    IParameterProvider& parameterProvider_;
};