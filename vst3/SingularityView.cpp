#include "SingularityView.h"


namespace Steinberg {

SingularityView::SingularityView(IParameterProvider& params)
    : m_params(params) {

    app_ = std::make_unique<visage::ApplicationWindow>();
    auto renderer = IRenderer::createRenderer(app_.get());
    auto jsEngine = IJSEngine::createJSEngine();
    auto fileWathcer = IFileWatcher::createFileWatcher(UI_DIR);
    controller_ = std::make_unique<SingularityController>(std::move(renderer), std::move(jsEngine), std::move(fileWathcer));


#ifndef NDEBUG
    hotReloadtimer.startTimer(50);
    hotReloadtimer.onTimerCallback().add([&]{
        auto prevWidth  = app_->width();
        auto prevHeight = app_->height();
        controller_->tick();
        if (frame) {
            auto newWidth  = app_->width();
            auto newHeight = app_->height();
            if (newWidth != prevWidth || newHeight != prevHeight) {
                ViewRect rect{0, 0, (int32)newWidth, (int32)newHeight};
                frame->resizeView(this, &rect);
            }
        }
    });
#endif

    controller_->initialize();
    currentWidth = app_->width();
    currentHeight = app_->height();
}

SingularityView::~SingularityView() { removed(); }

tresult PLUGIN_API SingularityView::isPlatformTypeSupported(FIDString type)
{
    fprintf(stderr, "[SingularityView] isPlatformTypeSupported: %s\n", type);
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
    fprintf(stderr, "[SingularityView] attached: type=%s parent=%p\n", type, parent);

    app_->show(visage::Dimension::nativePixels(app_->width()),
            visage::Dimension::nativePixels(app_->height()), parent);


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
    fprintf(stderr, "[SingularityView] getSize: %dx%d\n", currentWidth, currentHeight);
    if (!rect) return kResultFalse;
    rect->left = 0; rect->top = 0; rect->right = currentWidth; rect->bottom = currentHeight;

    return kResultOk;
}

tresult PLUGIN_API SingularityView::onSize(ViewRect* rect) {
    if (rect && app_)
    {
        currentWidth = rect->right - rect->left;
        currentHeight = rect->bottom - rect->top;
        app_->setNativeWindowDimensions(rect->right - rect->left, rect->bottom - rect->top);
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
