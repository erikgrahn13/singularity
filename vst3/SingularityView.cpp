#include "SingularityView.h"
#include "base/source/fdebug.h"

namespace Steinberg {

SingularityView::SingularityView(IParameterProvider& params)
    : m_params(params) {

    app_ = std::make_unique<visage::ApplicationWindow>();
    controller_ = std::make_unique<SingularityController>(app_.get(), params);

#ifndef NDEBUG
    hotReloadtimer.startTimer(50);
    hotReloadtimer.onTimerCallback().add([&]{
        controller_->tick();
        if (frame) {
            auto newWidth  = app_->width();
            auto newHeight = app_->height();
            if (newWidth != currentWidth || newHeight != currentHeight) {
                ViewRect rect{0, 0, (int32)newWidth, (int32)newHeight};
                frame->resizeView(this, &rect);
            }
        }
    });
#endif

    controller_->setLogger([](const std::string& msg) {
        SMTG_DBPRT1("%s\n", msg.c_str());
    });
    
    controller_->initialize();
    currentWidth = static_cast<visage::ApplicationWindow*>(controller_->getRootFrame())->width();
    currentHeight = static_cast<visage::ApplicationWindow*>(controller_->getRootFrame())->height();
}

SingularityView::~SingularityView() { removed(); }

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

tresult PLUGIN_API SingularityView::attached(void* parent, FIDString type)
{
    app_->show(visage::Dimension::logicalPixels(currentWidth),
            visage::Dimension::logicalPixels(currentHeight), parent);

    return kResultOk;
}

tresult PLUGIN_API SingularityView::removed()
{
    // m_win.reset();
    // m_graphics.reset();
    // TODO implement corret remove routine
    return kResultOk;
}

tresult PLUGIN_API SingularityView::getSize(ViewRect* rect)
{
    // fprintf(stderr, "[SingularityView] getSize: %dx%d\n", currentWidth, currentHeight);
    if (!rect) return kResultFalse;
    rect->left = 0; rect->top = 0; rect->right = currentWidth; rect->bottom = currentHeight;

    return kResultOk;
}

tresult PLUGIN_API SingularityView::onSize(ViewRect* rect) {
    if (rect && app_)
    {
        currentWidth = rect->right - rect->left;
        currentHeight = rect->bottom - rect->top;
        app_->setWindowDimensions(currentWidth, currentHeight);
    }
    return kResultOk;
}

tresult PLUGIN_API SingularityView::queryInterface(const TUID iid, void** obj)
{
    if (memcmp(iid, IPlugView::iid, sizeof(TUID)) == 0) { *obj = static_cast<IPlugView*>(this); addRef(); return kResultOk; }
    *obj = nullptr;
    return kNoInterface;
}

} // namespace Steinberg
