#pragma once

#include <memory>

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"

#include "SingularityEditor2.h"


class SingularityVST3Editor : public Steinberg::Vst::EditorView, 
                              public Steinberg::IPlugViewContentScaleSupport,
                              public SingularityEditor
#if defined(__linux__)
                              , public Steinberg::Linux::IEventHandler
#endif
{
    public:
    explicit SingularityVST3Editor(Steinberg::Vst::EditController* controller); 

    double getParameter(unsigned int id) override;
    void setParameter(unsigned int id, double value) override;

    Steinberg::tresult PLUGIN_API setContentScaleFactor(ScaleFactor factor) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API isPlatformTypeSupported(Steinberg::FIDString type) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API canResize() SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API checkSizeConstraint(Steinberg::ViewRect* rectToCheck) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect* newSize) SMTG_OVERRIDE;
    

#if defined(__linux__)
    Steinberg::tresult PLUGIN_API setFrame(Steinberg::IPlugFrame* frame) SMTG_OVERRIDE;
    void PLUGIN_API onFDIsSet(Steinberg::Linux::FileDescriptor fd) SMTG_OVERRIDE;
    // Steinberg::tresult PLUGIN_API setContextHost(Steinberg::FUnknown* context) SMTG_OVERRIDE;
#endif
    DEFINE_INTERFACES
        DEF_INTERFACE(Steinberg::IPlugViewContentScaleSupport)
    END_DEFINE_INTERFACES(Steinberg::Vst::EditorView)

    REFCOUNT_METHODS(Steinberg::Vst::EditorView)

    protected:
    void requestResize(int width, int height) override;
    void dispatchAction(std::string_view name, const ActionValue& value) override;
    void attachedToParent() SMTG_OVERRIDE;
    void removedFromParent() SMTG_OVERRIDE;
};
