#include "SingularityController.h"
#include <iostream>

SingularityController::SingularityController(std::unique_ptr<IRenderer> renderer, std::unique_ptr<IJSEngine> jsEngine, std::unique_ptr<IFileWatcher> fileWatcher)
: renderer_(std::move(renderer)), jsEngine_(std::move(jsEngine)), fileWatcher_(std::move(fileWatcher))
{
}

void SingularityController::initialize()
{
    fileWatcher_->setCallback([this](const std::string& filePath) {
        std::cout << "File changed" << std::endl;
        reloadPending_ = true;
    });
    jsEngine_->load("./main.js", renderer_.get());
}

void SingularityController::tick()
{
    if (reloadPending_.exchange(false))
        reload();
}

void SingularityController::reload()
{
    std::cout << "Reload called" << std::endl;
    jsEngine_->load("./main.js", renderer_.get());
}
