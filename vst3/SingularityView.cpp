#include "SingularityView.h"
#include "vst3controller.h"
#include "base/source/fdebug.h"

#include <dlfcn.h>
#include <filesystem>

namespace Steinberg {

SingularityView::SingularityView(Vst::EditController* editController)
    : Vst::EditorView(editController)
{
    Dl_info info;
    static int anchor;
    dladdr(&anchor, &info);
    std::filesystem::path soPath(info.dli_fname);
    std::filesystem::path resourcePath = soPath.parent_path().parent_path() / "Resources";
    fprintf(stderr, "resources: %s\n", resourcePath.c_str());

    auto& params = static_cast<IParameterProvider&>(*static_cast<VST3Controller*>(editController));
    app_        = std::make_unique<visage::ApplicationWindow>();
    controller_ = std::make_unique<SingularityController>(app_.get(), params, resourcePath.string());

#ifndef NDEBUG
    hotReloadtimer.startTimer(50);
    hotReloadtimer.onTimerCallback().add([&]{
        controller_->tick();
        if (plugFrame) {
            auto newWidth  = static_cast<int32>(app_->width());
            auto newHeight = static_cast<int32>(app_->height());
            if (newWidth != rect.right - rect.left || newHeight != rect.bottom - rect.top) {
                ViewRect newRect{0, 0, newWidth, newHeight};
                setRect(newRect);
                plugFrame->resizeView(this, &newRect);
            }
        }
    });
#endif

    controller_->setLogger([](const std::string& msg) {
        SMTG_DBPRT1("%s\n", msg.c_str());
        fprintf(stderr, "[singularity] %s\n", msg.c_str());
        fflush(stderr);
    });

    controller_->initialize();

    auto* win = static_cast<visage::ApplicationWindow*>(controller_->getRootFrame());
    setRect({0, 0, static_cast<int32>(win->width()), static_cast<int32>(win->height())});
}

SingularityView::~SingularityView()
{
    hotReloadtimer.stopTimer();
    if (app_) app_->hide();
    controller_.reset();
    app_.reset();
}

tresult PLUGIN_API SingularityView::isPlatformTypeSupported(FIDString type)
{
#if _WIN32
    if (strcmp(type, kPlatformTypeHWND) == 0) return kResultTrue;
#elif __APPLE__
    if (strcmp(type, kPlatformTypeNSView) == 0) return kResultTrue;
#else
    if (strcmp(type, kPlatformTypeX11EmbedWindowID) == 0) return kResultTrue;
#endif
    return kResultFalse;
}

void SingularityView::attachedToParent()
{
    app_->show(visage::Dimension::logicalPixels(rect.right - rect.left),
               visage::Dimension::logicalPixels(rect.bottom - rect.top),
               systemWindow);

#if defined(__linux__)
    if (plugFrame && app_->window()) {
        Linux::IRunLoop* runLoop = nullptr;
        if (plugFrame->queryInterface(Linux::IRunLoop::iid, (void**)&runLoop) == kResultOk && runLoop) {
            runLoop->registerEventHandler(this, app_->window()->posixFd());
            runLoop->release();
        }
    }
#endif

    Vst::EditorView::attachedToParent(); // notifies EditController
}

void SingularityView::removedFromParent()
{
#if defined(__linux__)
    if (plugFrame && app_->window()) {
        Linux::IRunLoop* runLoop = nullptr;
        if (plugFrame->queryInterface(Linux::IRunLoop::iid, (void**)&runLoop) == kResultOk && runLoop) {
            runLoop->unregisterEventHandler(this);
            runLoop->release();
        }
    }
#endif

    Vst::EditorView::removedFromParent(); // notifies EditController
}

tresult PLUGIN_API SingularityView::onSize(ViewRect* newSize)
{
    tresult res = CPluginView::onSize(newSize);
    if (res == kResultTrue && app_)
        app_->setWindowDimensions(rect.right - rect.left, rect.bottom - rect.top);
    return res;
}

} // namespace Steinberg
