#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "SingularityController.h"
#include "../platform/IWindow.h"
#include <memory>

namespace Steinberg {

class SingularityView : public Vst::EditorView
#if defined(__linux__)
                      , public Linux::IEventHandler
                      , public Linux::ITimerHandler
#endif
{
public:
    explicit SingularityView(Vst::EditController* controller);
    ~SingularityView() override;

    // EditorView / CPluginView overrides
    tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override;
    tresult PLUGIN_API onSize(ViewRect* newSize) override;
    tresult PLUGIN_API canResize() override { return kResultTrue; }
    tresult PLUGIN_API checkSizeConstraint(ViewRect*) override { return kResultTrue; }

    OBJ_METHODS(SingularityView, Vst::EditorView)
    DEFINE_INTERFACES
        DEF_INTERFACE(IPlugView)
    END_DEFINE_INTERFACES(Vst::EditorView)
    REFCOUNT_METHODS(Vst::EditorView)

protected:
    void attachedToParent() override;
    void removedFromParent() override;

#if defined(__linux__)
    // Linux::IEventHandler
    void PLUGIN_API onFDIsSet(Linux::FileDescriptor fd) override {
        window_->processEvents();
    }
    // Linux::ITimerHandler
    void PLUGIN_API onTimer() override {
        controller_->tick();
    }
#endif

private:
    // std::unique_ptr<visage::ApplicationWindow> app_;
    std::unique_ptr<IWindow> window_;
    std::unique_ptr<SingularityController>     controller_;
    // visage::EventTimer                         hotReloadtimer;
};

} // namespace Steinberg
