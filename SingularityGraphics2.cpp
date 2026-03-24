#include "SingularityGraphics2.h"
#include "IRenderer.h"
#include "IJSEngine.h"
#include "IFileWatcher.h"

#include <functional>
#include <string>
#include <iostream>
//#include <print>

// Factory function declarations (C++-only, Swift never sees .cpp)
std::unique_ptr<IRenderer> createRenderer(int width, int height);
std::unique_ptr<IFileWatcher> createFileWatcher(const std::string &directory, std::function<void(const std::string &filePath)> onChange);
std::unique_ptr<IJSEngine> createJSEngine();

SingularityGraphics::SingularityGraphics(int width, int height)
{
    renderer = createRenderer(width, height);
    jsEngine = createJSEngine();

    jsEngine->bindRenderer(renderer.get());

    // TODO: fix this function
    fileWatcher = createFileWatcher(JS_SCRIPTS_DIR, [this](const std::string &filePath){
        // jsEngine->hotReload();
        //std::println("Erik {}", filePath);
        pendingReload = true;
        // std::cout << "File has been modified" << std::endl;
    });
}

DrawingContent SingularityGraphics::getRenderData()
{
    return renderer->getDrawingContent();
}

void SingularityGraphics::hotReload()
{
    renderer->clear();
    jsEngine->hotReload();
}

void SingularityGraphics::onMouseDown(float x, float y)
{
    // std::println("onMouseDown1 x:{}    y:{}", x, y);
    jsEngine->onMouseDown(x, y);
}

void SingularityGraphics::renderFrame(float t)
{
    renderer->renderBackground(t);  // draws metaballs
    // jsEngine->render();             // JS widgets draw on top
    // jsEngine->hotReload();

}
