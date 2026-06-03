#pragma once
#include "IRenderer2.h"
#include "IJSEngine.h"
#include "IFileWatcher.h"
#include "IParameterProvider.h"
#include "platform/IWindow.h"
#include <atomic>
#include <string_view>

class SingularityController {
    public:
    SingularityController(IParameterProvider &parameterProvider, std::string_view resourcePath = "");
    void initialize();
    void tick(); // call from main thread each frame
    void setLogger(IJSEngine::LogCallback cb);
    int width() { return renderer_->width(); };
    int height() { return renderer_->height(); };
    void setOnResize(std::function<void(int, int)> cb) {
        renderer_->setOnResize(std::move(cb));
    }


    void attachToWindow(IWindow& window) {
        renderer_->attachToWindow(window);
        window.setOnMouseDown([this](int x, int y) {
            jsEngine_->onMouseDown((float)x, (float)y);
        });
        window.setOnMouseUp([this](int x, int y) {
            jsEngine_->onMouseUp((float)x, (float)y);
        });
        window.setOnMouseMove([this](int x, int y) {
            jsEngine_->onMouseMove((float)x, (float)y);
        });
    }
    void registerImage(const std::string& name, const uint8_t* data, int size);

    private:
    void reload();

    std::unique_ptr<IRenderer> renderer_;
    std::unique_ptr<IJSEngine> jsEngine_;
    std::unique_ptr<IFileWatcher> fileWatcher_;
    std::unique_ptr<IFileWatcher> widgetsWatcher_;
    std::atomic<bool> reloadPending_{false};
    IParameterProvider& parameterProvider_;
};