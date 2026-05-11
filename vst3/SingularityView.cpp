#include "SingularityView.h"
#include "vst3controller.h"
#include "base/source/fdebug.h"

namespace Steinberg {

SingularityView::SingularityView(Vst::EditController* editController)
    : Vst::EditorView(editController)
{
    // Create the app and controller now (before attached()) so getSize() can
    // return the real JS-driven dimensions when the host queries it.
    auto& params = static_cast<IParameterProvider&>(*static_cast<VST3Controller*>(editController));
    app_        = std::make_unique<visage::ApplicationWindow>();
    controller_ = std::make_unique<SingularityController>(app_.get(), params);
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
    Vst::EditorView::attachedToParent(); // notifies EditController

    // app_ and controller_ are already initialized; just show in the parent.
    app_->show(systemWindow);

#ifndef NDEBUG
    hotReloadtimer.startTimer(50);
    hotReloadtimer.onTimerCallback().add([&]{
        controller_->tick();
        if (plugFrame) {
            int newW = app_->width();
            int newH = app_->height();
            if (newW != rect.getWidth() || newH != rect.getHeight()) {
                rect = {0, 0, newW, newH};
                plugFrame->resizeView(this, &rect);
            }
        }
    });
#endif
}

void SingularityView::removedFromParent()
{
    Vst::EditorView::removedFromParent(); // notifies EditController

    hotReloadtimer.stopTimer();
    app_->hide();

    // Recreate for the next attach so JS re-runs cleanly.
    controller_.reset();
    app_.reset();
}

tresult PLUGIN_API SingularityView::onSize(ViewRect* newSize)
{
    CPluginView::onSize(newSize); // stores in rect
    if (app_)
        app_->setWindowDimensions(rect.getWidth(), rect.getHeight());
    return kResultTrue;
}

} // namespace Steinberg
