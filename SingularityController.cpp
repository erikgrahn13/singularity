#include "SingularityController.h"
#include <iostream>

SingularityController::SingularityController(IParameterProvider &parameterProvider, std::string_view resourcePath)
: parameterProvider_(parameterProvider)
{
    renderer_ = IRenderer::createRenderer(resourcePath);
    jsEngine_ = IJSEngine::createJSEngine(parameterProvider_);
    fileWatcher_ = IFileWatcher::createFileWatcher(UI_DIR);
    widgetsWatcher_ = IFileWatcher::createFileWatcher(SINGULARITY_WIDGETS_DIR);
}

void SingularityController::setLogger(IJSEngine::LogCallback cb)
{
    jsEngine_->setLogger(std::move(cb));
}

void SingularityController::initialize()
{
    fileWatcher_->setCallback([this](const std::string& filePath) {
        std::cout << "File changed" << std::endl;
        reloadPending_ = true;
    });
    widgetsWatcher_->setCallback([this](const std::string& filePath) {
        std::cout << "Widget changed: " << filePath << std::endl;
        reloadPending_ = true;
    });
    jsEngine_->load(UI_MAIN, renderer_.get());
}

void SingularityController::tick()
{
    if (reloadPending_.exchange(false))
        reload();

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
