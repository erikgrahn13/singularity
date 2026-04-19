// #include "NativeWindow.h"
// #include "../SingularityGraphics2.h"
// #include "../IParameterProvider.h"
// #include "ISingularityAudio.h"

// #include <limits>
// #include <memory>
// #include <unordered_map>

// int main()
// {
//     auto audio = ISingularityAudio::createSingularityAudio();
//     auto graphics = std::make_unique<SingularityGraphics>(PLUGIN_WIDTH, PLUGIN_HEIGHT, getParameterContainer(), /*standalone=*/true);
//     setOnParameterChanged([&](int id, double value) {
//         audio->pushParameterChange(id, value);
//     });


//     auto win = createNativeWindow("Singularity", PLUGIN_WIDTH, PLUGIN_HEIGHT);

//     // Settings window — created on demand, destroyed on close
//     std::unique_ptr<IWindow> settingsWin;
//     std::unique_ptr<SingularityGraphics> settingsGraphics;
//     bool settingsDirty = false;

//     graphics->setOnOpenSettings([&]() {
//         if (settingsWin) return; // already open

//         settingsGraphics = std::make_unique<SingularityGraphics>(400, 300, getParameterContainer(), true, std::string(JS_SCRIPTS_DIR) + "/widgets/settings.js");

//         settingsWin = createNativeWindow("Settings", 400, 300, nullptr, win.get());
//         settingsDirty = true;

//         settingsWin->setOnMouseDown([&](int x, int y, unsigned int) { settingsGraphics->onMouseDown((float)x, (float)y); settingsDirty = true; });
//         settingsWin->setOnMouseUp  ([&](int x, int y, unsigned int) { settingsGraphics->onMouseUp  ((float)x, (float)y); settingsDirty = true; });
//         settingsWin->setOnMouseMove([&](int x, int y)               { settingsGraphics->onMouseMove((float)x, (float)y); settingsDirty = true; });
//         settingsWin->setOnFrame([&]() -> DrawingContent {
//             if (!settingsDirty) return {};
//             settingsGraphics->renderUI();
//             settingsDirty = false;
//             return settingsGraphics->getRenderData();
//         });

//         settingsWin->setOnClose([&]() {
//             settingsGraphics.reset();
//             settingsWin.reset();
//         });
//     });

//     win->setOnMouseDown([&](int x, int y, unsigned int /*btn*/) { graphics->onMouseDown((float)x, (float)y); });
//     win->setOnMouseUp  ([&](int x, int y, unsigned int /*btn*/) { graphics->onMouseUp  ((float)x, (float)y); });
//     win->setOnMouseMove([&](int x, int y)                       { graphics->onMouseMove((float)x, (float)y); });

//     win->setOnFrame([&]() -> DrawingContent {
// #ifndef NDEBUG
//         if (graphics->pendingReload.exchange(false)) {
//             graphics->hotReload();
//         }
// #endif
//         graphics->renderUI();
//         return graphics->getRenderData();
//     });

//     win->run();
// }

#include <visage/app.h>
#include <visage_ui/events.h>

#include "ISingularityAudio.h"
#include "../SingularityGraphics2.h"

class ExampleEditor : public visage::ApplicationWindow, public visage::EventTimer {
public:
  explicit ExampleEditor(std::shared_ptr<SingularityGraphics> graphics)
      : graphics_(std::move(graphics)) {
    startTimer(100);  // poll for hot reload every 100 ms
  }

  void timerCallback() override {
#ifndef NDEBUG
    if (graphics_->pendingReload.exchange(false)) {
      graphics_->hotReload();
      redraw();
    }
#endif
  }

  void draw(visage::Canvas& canvas) override {
    graphics_->setCanvas(static_cast<void*>(&canvas));
    graphics_->renderUI();
    graphics_->setCanvas(nullptr);
  }

  void mouseMove(const visage::MouseEvent& e) override {
    graphics_->onMouseMove(e.position.x, e.position.y);
    redraw();
  }
  void mouseDrag(const visage::MouseEvent& e) override {
    graphics_->onMouseMove(e.position.x, e.position.y);
    redraw();
  }
  void mouseDown(const visage::MouseEvent& e) override {
    graphics_->onMouseDown(e.position.x, e.position.y);
    redraw();
  }
  void mouseUp(const visage::MouseEvent& e) override {
    graphics_->onMouseUp(e.position.x, e.position.y);
    redraw();
  }

private:
  std::shared_ptr<SingularityGraphics> graphics_;
};

int main()
{
  auto audio    = ISingularityAudio::createSingularityAudio();
  auto graphics = std::make_shared<SingularityGraphics>(
      PLUGIN_WIDTH, PLUGIN_HEIGHT, getParameterContainer(), /*standalone=*/true);

  setOnParameterChanged([&](int id, double value) {
    audio->pushParameterChange(id, value);
  });

  ExampleEditor editor(graphics);
  editor.show(visage::Dimension::logicalPixels(PLUGIN_WIDTH),
              visage::Dimension::logicalPixels(PLUGIN_HEIGHT));
  editor.runEventLoop();

  return 0;
}

