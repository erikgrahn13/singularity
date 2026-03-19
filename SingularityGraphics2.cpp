#include "SingularityGraphics2.h"
#include "IRenderer.h"
#include "IJSEngine.h"
#include "IFileWatcher.h"

#include <functional>
#include <string>
#include <iostream>

// Factory function declarations (C++-only, Swift never sees .cpp)
std::unique_ptr<IRenderer> createRenderer(int width, int height);
std::unique_ptr<IFileWatcher> createFileWatcher(const std::string &directory, std::function<void(const std::string &filePath)> onChange);
std::unique_ptr<IJSEngine> createJSEngine();

SingularityGraphics::SingularityGraphics(int width, int height)
{
    renderer = createRenderer(width, height);
    jsEngine = createJSEngine();

    // TODO: fix this function
    fileWatcher = createFileWatcher(JS_SCRIPTS_DIR, [](const std::string &filePath){
        std::cout << "File has been modified" << std::endl;
    });
}

DrawingContent SingularityGraphics::getRenderData()
{
    return renderer->getDrawingContent();
}