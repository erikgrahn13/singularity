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
#if __has_include("embedded/generated_resources.h")
#include "embedded/generated_resources.h"
namespace singularity { namespace generated { ::visage::EmbeddedFile fileByName(const std::string& filename); } }
#endif

#include <filesystem>
#include <algorithm>


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

  // Explicitly register embedded resources for standalone builds.
#if __has_include("embedded/generated_resources.h")
  {
    namespace fs = std::filesystem;
    std::string resdir = UI_RESOURCES_DIR;
    if (!resdir.empty() && fs::exists(resdir) && fs::is_directory(resdir)) {
      for (auto &entry : fs::directory_iterator(resdir)) {
        if (!entry.is_regular_file()) continue;
        auto name = entry.path().filename().string();

        // sanitize: replace '.', ' ', '-' with '_'
        std::string key = name;
        std::replace_if(key.begin(), key.end(), [](char c){ return c=='.' || c==' ' || c=='-'; }, '_');

        auto f = singularity::generated::fileByName(key);
        if (f.name) {
          controller->registerImage(std::string(f.name), f.data, f.size);
          controller->registerImage(name, f.data, f.size);
        }
      }
    }
  }
#endif

  controller->initialize();

  auto width  = static_cast<visage::ApplicationWindow*>(controller->getRootFrame())->width();
  auto height = static_cast<visage::ApplicationWindow*>(controller->getRootFrame())->height();

  app.show(visage::Dimension::logicalPixels(800),
           visage::Dimension::logicalPixels(600));
  
  app.runEventLoop();

  return 0;
}


