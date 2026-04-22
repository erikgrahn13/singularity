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
#include <visage_graphics/post_effects.h>

#include "ISingularityAudio.h"
#include "../SingularityController.h"
#include "../IRenderer2.h"
#include "../IJSEngine.h"
#include "../IFileWatcher.h"

#include <iostream>

// class SingularityWindow : public visage::ApplicationWindow, public visage::EventTimer {
// public:
//   explicit SingularityWindow(std::shared_ptr<SingularityGraphics> graphics)
//       : graphics_(std::move(graphics)) {
// // #ifndef NDEBUG
// //     startTimer(50);
// // #endif
// //     onDraw() = [this](visage::Canvas& canvas) {
// //       graphics_->renderUI(canvas);
// //         redraw();
// //     };

// //     graphics_->setOnSetBloom([this](float size, float intensity) {
// //       bloom_.setBloomSize(size);
// //       bloom_.setBloomIntensity(intensity);
// //       setPostEffect(size > 0 ? &bloom_ : nullptr);
// //     });
// //   }

// //   void mouseMove(const visage::MouseEvent& e) override {
// //     graphics_->onMouseMove(e.position.x, e.position.y);
// //     // redraw();
// //   }
// //   void mouseDrag(const visage::MouseEvent& e) override {
// //     graphics_->onMouseMove(e.position.x, e.position.y);
// //     // redraw();
// //   }
// //   void mouseDown(const visage::MouseEvent& e) override {
// //     graphics_->onMouseDown(e.position.x, e.position.y);
// //     // redraw();
// //   }
// //   void mouseUp(const visage::MouseEvent& e) override {
// //     graphics_->onMouseUp(e.position.x, e.position.y);
// //     // redraw();
// //   }

// //   // Only fired in debug builds for hot reloading
// //   void timerCallback() override {
// //     if (graphics_->pendingReload.exchange(false)) {
// //       graphics_->hotReload();
// //       redraw();
// //     }
//   }

// private:
//   std::shared_ptr<SingularityGraphics> graphics_;
//   visage::BloomPostEffect bloom_;
// };

// int main()
// {
//   visage::ApplicationWindow app;

// //   auto audio    = ISingularityAudio::createSingularityAudio();
// //   auto graphics = std::make_shared<SingularityGraphics>(
// //       PLUGIN_WIDTH, PLUGIN_HEIGHT, getParameterContainer(), true);

// // //   setOnParameterChanged([&](int id, double value) {
// // //     audio->pushParameterChange(id, value);
// // //   });

// //   SingularityWindow window(graphics);
//   app.show(visage::Dimension::logicalPixels(PLUGIN_WIDTH),
//               visage::Dimension::logicalPixels(PLUGIN_HEIGHT));
//   app.runEventLoop();

//   return 0;
// }

int main()
{
  visage::ApplicationWindow app;

  auto renderer = IRenderer::createRenderer(&app);
  auto jsEngine = IJSEngine::createJSEngine();
  auto fileWathcer = IFileWatcher::createFileWatcher(UI_DIR);

  auto controller = std::make_unique<SingularityController>(std::move(renderer), std::move(jsEngine), std::move(fileWathcer));

#ifndef NDEBUG
    visage::EventTimer timer;
    timer.startTimer(50);
    timer.onTimerCallback().add([&]{
        controller->tick();
    });
#endif

  app.show(visage::Dimension::logicalPixels(PLUGIN_WIDTH),
  visage::Dimension::logicalPixels(PLUGIN_HEIGHT));
  
  controller->initialize();
  app.runEventLoop();

  return 0;
}


