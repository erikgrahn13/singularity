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
#include "embedded/generated_resources.h"
#include <filesystem>
#include <algorithm>

// Some generated lookup implementations use `fileByName` in the generated cpp,
// so forward-declare it here to be safe.
namespace singularity { namespace generated { ::visage::EmbeddedFile fileByName(const std::string& filename); } }


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

  controller->initialize();

  auto width  = static_cast<visage::ApplicationWindow*>(controller->getRootFrame())->width();
  auto height = static_cast<visage::ApplicationWindow*>(controller->getRootFrame())->height();

  app.show(visage::Dimension::logicalPixels(800),
           visage::Dimension::logicalPixels(600));
  
  app.runEventLoop();

  return 0;
}


