#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "SingularityController.h"
#include PLUGIN_CLASS_HEADER

#include <memory>
#include <QQuickView>

namespace Steinberg {

class SingularityView : public Vst::EditorView
#if defined(__linux__)
                      , public Linux::IEventHandler
                      , public Linux::ITimerHandler
#endif
{
public:
    explicit SingularityView(Vst::EditController* controller);
    ~SingularityView() override = default;

    tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override;
    tresult PLUGIN_API onSize(ViewRect* newSize) override;
    tresult PLUGIN_API canResize() override { return PLUGIN_CLASS::isResizable ? kResultTrue : kResultFalse; }
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
    void PLUGIN_API onFDIsSet(Linux::FileDescriptor fd) override;
    void PLUGIN_API onTimer() override;
#endif

private:
    std::unique_ptr<QQuickView> view_;
    std::unique_ptr<QWindow> parentWindow_;
    std::unique_ptr<SingularityController> controller_;
#if defined(__linux__)
    Linux::FileDescriptor qtXcbFd_ = -1;
    bool eventHandlerRegistered_ = false;
    bool timerRegistered_ = false;
#endif
};

} // namespace Steinberg
