#pragma once

#if __has_include(<swift/bridging>)
#  include <swift/bridging>
#else
#  define SWIFT_RETURNS_INDEPENDENT_VALUE
#endif

#include <memory>
#include <atomic>
// Include interface headers - they're lightweight (just virtual functions)
#include "IRenderer.h"
#include "IJSEngine.h"
#include "IFileWatcher.h"

class SingularityGraphics {


    public:
    SingularityGraphics(int width, int height);

    SWIFT_RETURNS_INDEPENDENT_VALUE
    DrawingContent getRenderData();

    void hotReload();

    std::atomic<bool> pendingReload { true };

    private:
    std::unique_ptr<IRenderer> renderer;
    std::unique_ptr<IJSEngine> jsEngine;
    std::unique_ptr<IFileWatcher> fileWatcher;
};
