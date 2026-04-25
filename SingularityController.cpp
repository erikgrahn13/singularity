#include "SingularityController.h"
#include <iostream>

SingularityController::SingularityController(void *rootFrame, IParameterProvider &parameterProvider)
: parameterProvider_(parameterProvider)
{
    renderer_ = IRenderer::createRenderer(rootFrame);
    jsEngine_ = IJSEngine::createJSEngine();
    fileWatcher_ = IFileWatcher::createFileWatcher(UI_DIR);
}

void SingularityController::initialize()
{
    renderer_->setComponentMouseDownCallback(
        [this](void* component, float x, float y) {
            jsEngine_->onMouseDown(component, x, y);
        }
    );

    renderer_->setComponentMouseUpCallback(
        [this](void* component, float x, float y) {
            jsEngine_->onMouseUp(component, x, y);
        }
    );

    renderer_->setComponentMouseDragCallback(
        [this](void* component, float x, float y) {
            jsEngine_->onMouseDrag(component, x, y);
        }
    );

    fileWatcher_->setCallback([this](const std::string& filePath) {
        std::cout << "File changed" << std::endl;
        reloadPending_ = true;
    });
    jsEngine_->load(UI_MAIN, renderer_.get());
}

void SingularityController::tick()
{
    if (reloadPending_.exchange(false))
        reload();
}

void SingularityController::reload()
{
    std::cout << "Reload called" << std::endl;
    jsEngine_->load(UI_MAIN, renderer_.get());
}
