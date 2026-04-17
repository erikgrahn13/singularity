#include "SingularityGraphics2.h"
#include "IRenderer.h"
#include "IJSEngine.h"
#include "IFileWatcher.h"

#include <functional>
#include <string>
#include <iostream>
// #include <print>

// Factory function declarations (C++-only, Swift never sees .cpp)
std::unique_ptr<IRenderer> createRenderer(int width, int height);
std::unique_ptr<IFileWatcher> createFileWatcher(const std::string &directory, std::function<void(const std::string &filePath)> onChange);
std::unique_ptr<IJSEngine> createJSEngine(IRenderer *renderer, IParameterProvider &parameterStore, bool standalone);

SingularityGraphics::SingularityGraphics(int width, int height, IParameterProvider &parameterProvider,
                                         bool standalone, const std::string& scriptPath)
: parameterProvider(parameterProvider)
{
    renderer = createRenderer(width, height);
    jsEngine = createJSEngine(renderer.get(), parameterProvider, standalone);

    fileWatcher = createFileWatcher(JS_SCRIPTS_DIR, [this](const std::string &filePath){
        pendingReload = true;
    });

    if (!scriptPath.empty())
        jsEngine->loadScript(scriptPath);
}

DrawingContent SingularityGraphics::getRenderData()
{
    return renderer->getDrawingContent();
}

void SingularityGraphics::hotReload()
{
    jsEngine->hotReload();
}

void SingularityGraphics::loadScript(const std::string& path)
{
    jsEngine->loadScript(path);
}

void SingularityGraphics::setStringList(const std::string& key, std::vector<std::string> values)
{
    jsEngine->setStringList(key, std::move(values));
}

void SingularityGraphics::addParameter()
{

}

void SingularityGraphics::setOnOpenSettings(std::function<void()> cb)
{
    onOpenSettings = cb;
    jsEngine->setOnOpenSettings(std::move(cb));
}

void SingularityGraphics::onMouseDown(float x, float y)
{
    // std::println("onMouseDown1 x:{}    y:{}", x, y);
    jsEngine->onMouseDown(x, y);
}

void SingularityGraphics::onMouseUp(float x, float y)
{
    jsEngine->onMouseUp(x, y);
}

void SingularityGraphics::onMouseMove(float x, float y)
{
    jsEngine->onMouseMove(x, y);

}

void SingularityGraphics::renderUI()
{
    jsEngine->renderUI();
}

void SingularityGraphics::renderFrame(float t)
{
    renderer->renderBackground(t);  // draws metaballs
    // jsEngine->render();             // JS widgets draw on top
    // jsEngine->hotReload();

}
