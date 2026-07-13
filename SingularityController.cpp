#include "SingularityController.h"
#include <iostream>

SingularityController::SingularityController(IParameterProvider &parameterProvider, std::string_view resourcePath, Singularity::AudioDataExchange::AudioDataQueue* audioDataQueue)
: parameterProvider_(parameterProvider)
{
    renderer_ = IRenderer::createRenderer(resourcePath);
    jsEngine_ = IJSEngine::createJSEngine(parameterProvider_, audioDataQueue);

#ifndef NDEBUG
    fileWatcher_ = IFileWatcher::createFileWatcher(UI_DIR);
    widgetsWatcher_ = IFileWatcher::createFileWatcher(SINGULARITY_WIDGETS_DIR);
    fprintf(stderr, "[singularity] Watching: %s\n", UI_DIR);
#endif
}

void SingularityController::setLogger(IJSEngine::LogCallback cb)
{
    jsEngine_->setLogger(std::move(cb));
}

void SingularityController::initialize()
{
#ifndef NDEBUG
    fileWatcher_->setCallback([this](const std::string& filePath) {
        std::cout << "File changed" << std::endl;
        reloadPending_ = true;
    });
    widgetsWatcher_->setCallback([this](const std::string& filePath) {
        std::cout << "Widget changed: " << filePath << std::endl;
        reloadPending_ = true;
    });
#endif
    jsEngine_->load(UI_MAIN, renderer_.get());
}

void SingularityController::tick()
{
#ifndef NDEBUG
    if (reloadPending_.exchange(false)) {
        reload();
        dirty_ = true;
    }
#endif

    // Periodic redraw to pick up host-automated parameter changes.
    // Every ~20 frames (~3Hz at 60fps) we force a redraw.
    if (++frameCounter_ >= 20) {
        frameCounter_ = 0;
        dirty_ = true;
    }

    bool shouldDraw = dirty_.exchange(false) || jsEngine_->wantsAnimatedRedraw();
    if (!shouldDraw) return;

    renderer_->beginFrame();
    if (!renderer_->currentCanvas()) return;
    jsEngine_->draw();
    renderer_->present();
}

void SingularityController::reload()
{
    std::cout << "Reload called" << std::endl;
    renderer_->clearImageCache();
    jsEngine_->load(UI_MAIN, renderer_.get());
}

void SingularityController::registerImage(const std::string& name, const uint8_t* data, int size)
{
    renderer_->registerImage(name, data, size);
}
