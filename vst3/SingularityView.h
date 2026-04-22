#pragma once

#include "pluginterfaces/gui/iplugview.h"
#include "SingularityController.h"
#include "IParameterProvider.h"
#include <memory>

#include <visage/app.h>
namespace Steinberg {

class SingularityView : public IPlugView
{
public:
    SingularityView(IParameterProvider& params);
    ~SingularityView();

    // IPlugView
    tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override;
    tresult PLUGIN_API attached(void* parent, FIDString type) override;
    tresult PLUGIN_API removed() override;
    tresult PLUGIN_API onWheel(float) override                     { return kResultFalse; }
    tresult PLUGIN_API onKeyDown(char16, int16, int16) override     { return kResultFalse; }
    tresult PLUGIN_API onKeyUp(char16, int16, int16) override       { return kResultFalse; }
    tresult PLUGIN_API getSize(ViewRect* rect) override;
    tresult PLUGIN_API onSize(ViewRect*) override;
    tresult PLUGIN_API onFocus(TBool) override                      { return kResultOk; }
    tresult PLUGIN_API setFrame(IPlugFrame* f) override             { frame = f; return kResultOk; }
    tresult PLUGIN_API canResize() override                         { return kResultTrue; }
    tresult PLUGIN_API checkSizeConstraint(ViewRect*) override      { return kResultOk; }

    // IUnknown
    tresult PLUGIN_API queryInterface(const TUID iid, void** obj) override;
    uint32  PLUGIN_API addRef() override  { return ++refCount; }
    uint32  PLUGIN_API release() override { if (--refCount == 0) { delete this; return 0; } return refCount; }

private:
    IParameterProvider& m_params;
    IPlugFrame*         frame    = nullptr;
    uint32              refCount = 1;

    int currentWidth;
    int currentHeight;

    std::unique_ptr<SingularityController> controller_;
    std::unique_ptr<visage::ApplicationWindow> app_;
    visage::EventTimer hotReloadtimer;
};

} // namespace Steinberg
