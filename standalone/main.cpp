#include <iostream>

#include "ISingularityAudio.h"
#include "../SingularityController.h"
#include "../platform/IWindow.h"

#include PLUGIN_CLASS_HEADER

#ifdef SINGULARITY_HAS_EMBEDDED_RESOURCES
#include "generated_resources.h"
#endif

#if defined(__linux__)
#include "PipeWire.h"
using PlatformAudio = PipeWire<PLUGIN_CLASS>;
#elif defined(__APPLE__)
#include "coreAudio.h"
using PlatformAudio = CoreAudio<PLUGIN_CLASS>;
#elif defined(_WIN32)
#include "ASIO.h"
using PlatformAudio = ASIO<PLUGIN_CLASS>;
#endif


int main()
{
    std::cout << "Hello main2" << std::endl;
    auto audio = std::make_unique<PlatformAudio>();
    setOnParameterChanged([&](int id, double value) {
        audio->pushParameterChange(id, value);
    });

    auto controller = std::make_unique<SingularityController>(getParameterContainer(), UI_RESOURCES_DIR);
    controller->setLogger([](const std::string& msg) {
        std::cout << msg << std::endl;
    });
    controller->initialize();

#ifdef SINGULARITY_HAS_EMBEDDED_RESOURCES
    singularity_register_images(controller.get());
#endif

    auto width  = controller->width();
    auto height = controller->height();

    auto window = IWindow::createWindow(width, height);
    window->setResizable(PLUGIN_CLASS::isResizable);
    controller->attachToWindow(*window);

    controller->setOnResize([&](int w, int h) {
        window->resize(w, h);
    });

    window->setOnFrame([&]() {
        controller->tick();  // checks reloadPending_, calls reload() if set
    });

    window->run();

    return 0;
}