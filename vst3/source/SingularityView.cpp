#include "SingularityView.h"

#if _WIN32
#  include "standalone/windows/Win32Window.h"
#endif

namespace Steinberg {

SingularityView::SingularityView(IParameterProvider& params, int width, int height)
    : m_params(params), m_width(width), m_height(height) {}

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

tresult PLUGIN_API SingularityView::attached(void* parent, FIDString /*type*/)
{
#if _WIN32
    auto* win = new Win32Window("", m_width, m_height, static_cast<HWND>(parent), true);
    m_win.reset(win);

    m_graphics = std::make_unique<SingularityGraphics>(m_width, m_height, m_params);
    m_graphics->loadScript(std::string(JS_SCRIPTS_DIR) + "/hello.js");
    m_dirty = true;

    m_win->setOnMouseDown([&](int x, int y, unsigned int) { m_graphics->onMouseDown((float)x, (float)y); m_dirty = true; });
    m_win->setOnMouseUp  ([&](int x, int y, unsigned int) { m_graphics->onMouseUp  ((float)x, (float)y); m_dirty = true; });
    m_win->setOnMouseMove([&](int x, int y)               { m_graphics->onMouseMove((float)x, (float)y); m_dirty = true; });
    m_win->setOnFrame([&]() -> DrawingContent {
        if (!m_dirty) return {};
        m_graphics->renderUI();
        m_dirty = false;
        return m_graphics->getRenderData();
    });

    win->startTimer();
#endif
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
