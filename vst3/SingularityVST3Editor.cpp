#include "SingularityVST3Editor.h"

SingularityVST3Editor::SingularityVST3Editor(Steinberg::Vst::EditController* controller)
    : Steinberg::Vst::EditorView(controller)
{
    // setRect({0, 0, 100, 100});

    // auto vst3Controller = static_cast<VST3Controller*>(controller);

}

void SingularityVST3Editor::attachedToParent()
{
    // view_ = std::make_unique<QQuickView>();

    // view_->loadFromModule("ExampleEffect", "Main");

    // view_->setResizeMode(
    //     QQuickView::SizeRootObjectToView
    // );

    // view_->resize(800, 600);
    // view_->show();
    
}

void SingularityVST3Editor::removedFromParent()
{

}

Steinberg::tresult PLUGIN_API SingularityVST3Editor::setContentScaleFactor(Steinberg::IPlugViewContentScaleSupport::ScaleFactor factor)
{
    return Steinberg::kResultOk;
}

Steinberg::tresult PLUGIN_API SingularityVST3Editor::isPlatformTypeSupported(Steinberg::FIDString type)
{
    #if SMTG_OS_WINDOWS
        if(strcmp(type, Steinberg::kPlatformTypeHWND) == 0)
            return Steinberg::kResultTrue;
    #elif SMTG_OS_MACOS
        if(strcmp(type, kPlatformTypeNSView) == 0)
            return Steinberg::kResultTrue;
    #elif SMTG_OS_LINUX
        if(strcmp(type, Steinberg::kPlatformTypeX11EmbedWindowID) == 0)
            return Steinberg::kResultTrue;
        if(strcmp(type, Steinberg::kPlatformString) == 0)
            return Steinberg::kResultTrue;
    #endif

    return Steinberg::kInvalidArgument;
}

