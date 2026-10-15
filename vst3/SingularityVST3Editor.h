#pragma once

#include <memory>

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"

#include "SingularityEditor.h"

#include <QQuickView>

class SingularityVST3Editor : public Steinberg::Vst::EditorView, 
                              public Steinberg::IPlugViewContentScaleSupport,
                              public SingularityEditor
{
    public:
    explicit SingularityVST3Editor(Steinberg::Vst::EditController* controller); 

    Steinberg::tresult PLUGIN_API setContentScaleFactor(ScaleFactor factor) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API isPlatformTypeSupported(Steinberg::FIDString type) SMTG_OVERRIDE;

    DEFINE_INTERFACES
        DEF_INTERFACE(Steinberg::IPlugViewContentScaleSupport)
    END_DEFINE_INTERFACES(Steinberg::Vst::EditorView)

    REFCOUNT_METHODS(Steinberg::Vst::EditorView)

    protected:
    // EditorView
    void attachedToParent() SMTG_OVERRIDE;
    void removedFromParent() SMTG_OVERRIDE;

    std::unique_ptr<QQuickView> view_;

    // IPlugView
};