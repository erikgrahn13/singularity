#pragma once
#include "IRenderer2.h"
#include "IJSEngine.h"
#include "IFileWatcher.h"
#include <atomic>

class SingularityController {
    public:
    SingularityController(std::unique_ptr<IRenderer> renderer, std::unique_ptr<IJSEngine> jsEngine, std::unique_ptr<IFileWatcher> fileWatcher);
    void initialize();
    void tick(); // call from main thread each frame

    private:
    void reload();

    std::unique_ptr<IRenderer> renderer_;
    std::unique_ptr<IJSEngine> jsEngine_;
    std::unique_ptr<IFileWatcher> fileWatcher_;
    std::atomic<bool> reloadPending_{false};
};