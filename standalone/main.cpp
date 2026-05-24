#include <visage/app.h>
#include <visage_ui/events.h>
#include <visage_graphics/post_effects.h>

#include "ISingularityAudio.h"
#include "../SingularityController.h"
#include "../IRenderer2.h"
#include "../IJSEngine.h"
#include "../IFileWatcher.h"
#include PLUGIN_CLASS_HEADER

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

#include <iostream>

#ifdef NDEBUG
#include "generated_images.h"
#endif

int main()
{
  visage::ApplicationWindow app;

  auto audio = std::make_unique<PlatformAudio>();
  setOnParameterChanged([&](int id, double value) {
    audio->pushParameterChange(id, value);
  });

  auto controller = std::make_unique<SingularityController>(&app, getParameterContainer(), UI_RESOURCES_DIR);

#if !defined NDEBUG
    visage::EventTimer timer;
    timer.startTimer(50);
    timer.onTimerCallback().add([&]{
        controller->tick();
    });
#endif


  controller->setLogger([](const std::string& msg) {
    std::cout << msg << std::endl;
  });

#ifdef NDEBUG
  singularity_register_images(controller.get());
#endif

  controller->initialize();

  auto width  = static_cast<visage::ApplicationWindow*>(controller->getRootFrame())->width();
  auto height = static_cast<visage::ApplicationWindow*>(controller->getRootFrame())->height();

  app.show(visage::Dimension::logicalPixels(800),
           visage::Dimension::logicalPixels(600));
  
  app.runEventLoop();

  return 0;
}


