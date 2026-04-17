#pragma once

#include "pluginterfaces/gui/iplugview.h"
#include "SingularityGraphics2.h"
#include "IParameterProvider.h"
#include "IWindow.h"
#include <memory>

namespace Steinberg {

class SingularityView : public IPlugView
{
public:
    SingularityView(IParameterProvider& params, int width, int height);
    ~SingularityView();

    // IPlugView
    tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override;
    tresult PLUGIN_API attached(void* parent, FIDString type) override;
    tresult PLUGIN_API removed() override;
    tresult PLUGIN_API onWheel(float) override                     { return kResultFalse; }
    tresult PLUGIN_API onKeyDown(char16, int16, int16) override     { return kResultFalse; }
    tresult PLUGIN_API onKeyUp(char16, int16, int16) override       { return kResultFalse; }
    tresult PLUGIN_API getSize(ViewRect* rect) override;
    tresult PLUGIN_API onSize(ViewRect*) override                   { return kResultOk; }
    tresult PLUGIN_API onFocus(TBool) override                      { return kResultOk; }
    tresult PLUGIN_API setFrame(IPlugFrame* f) override             { frame = f; return kResultOk; }
    tresult PLUGIN_API canResize() override                         { return kResultFalse; }
    tresult PLUGIN_API checkSizeConstraint(ViewRect*) override      { return kResultOk; }

    // IUnknown
    tresult PLUGIN_API queryInterface(const TUID iid, void** obj) override;
    uint32  PLUGIN_API addRef() override  { return ++refCount; }
    uint32  PLUGIN_API release() override { if (--refCount == 0) { delete this; return 0; } return refCount; }

private:
    IParameterProvider& m_params;
    int                 m_width;
    int                 m_height;
    IPlugFrame*         frame    = nullptr;
    uint32              refCount = 1;

    // Platform window + graphics — created in attached(), destroyed in removed()
    std::unique_ptr<IWindow>           m_win;
    std::unique_ptr<SingularityGraphics> m_graphics;
    bool m_dirty = true;
};

} // namespace Steinberg
