#include "SingularityView.h"
#include "platform/NativeWindow.h"

namespace Steinberg {

SingularityView::SingularityView(IParameterProvider& params, int width, int height)
    : m_params(params), m_width(width), m_height(height) {}

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

    m_win = createNativeWindow("", m_width, m_height, parent);
    m_graphics = std::make_unique<SingularityGraphics>(m_width, m_height, m_params);

    m_win->setOnMouseDown([&](int x, int y, unsigned int) { m_graphics->onMouseDown((float)x, (float)y); });
    m_win->setOnMouseUp  ([&](int x, int y, unsigned int) { m_graphics->onMouseUp  ((float)x, (float)y); });
    m_win->setOnMouseMove([&](int x, int y)               { m_graphics->onMouseMove((float)x, (float)y); });
    m_win->setOnFrame([&]() -> DrawingContent {
#ifndef NDEBUG
        if (m_graphics->pendingReload.exchange(false))
            m_graphics->hotReload();
#endif
        m_graphics->renderUI();
        return m_graphics->getRenderData();
    });

    return kResultOk;
}

tresult PLUGIN_API SingularityView::removed()
{
    m_win.reset();
    m_graphics.reset();
    return kResultOk;
}

tresult PLUGIN_API SingularityView::getSize(ViewRect* rect)
{
    fprintf(stderr, "[SingularityView] getSize: %dx%d\n", m_width, m_height);
    if (!rect) return kResultFalse;
    rect->left = 0; rect->top = 0; rect->right = m_width; rect->bottom = m_height;
    return kResultOk;
}

tresult PLUGIN_API SingularityView::queryInterface(const TUID iid, void** obj)
{
    if (memcmp(iid, IPlugView::iid, sizeof(TUID)) == 0) { *obj = static_cast<IPlugView*>(this); addRef(); return kResultOk; }
    *obj = nullptr;
    return kNoInterface;
}

} // namespace Steinberg
