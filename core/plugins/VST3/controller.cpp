//------------------------------------------------------------------------
// Copyright(c) 2025 FahlGrahn.
//------------------------------------------------------------------------

#include "controller.h"
#include "cids.h"
#include "vst3editor.h"

using namespace Steinberg;

namespace MyCompanyName
{

//------------------------------------------------------------------------
// SingularityEffectController Implementation
//------------------------------------------------------------------------
tresult PLUGIN_API SingularityEffectController::initialize(FUnknown *context)
{
    // Here the Plug-in will be instantiated

    //---do not forget to call parent ------
    tresult result = EditControllerEx1::initialize(context);
    if (result != kResultOk)
    {
        return result;
    }

    // audioEditor = createEditorInstance();
    // Here you could register some parameters

    return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API SingularityEffectController::terminate()
{
    // Here the Plug-in will be de-instantiated, last possibility to remove some memory!

    //---do not forget to call parent ------
    return EditControllerEx1::terminate();
}

//------------------------------------------------------------------------
tresult PLUGIN_API SingularityEffectController::setComponentState(IBStream *state)
{
    // Here you get the state of the component (Processor part)
    if (!state)
        return kResultFalse;

    return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API SingularityEffectController::setState(IBStream *state)
{
    // Here you get the state of the controller

    return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API SingularityEffectController::getState(IBStream *state)
{
    // Here you are asked to deliver the state of the controller (if needed)
    // Note: the real state of your plug-in is saved in the processor

    return kResultTrue;
}

//------------------------------------------------------------------------
IPlugView *PLUGIN_API SingularityEffectController::createView(FIDString name)
{
    // Here the Host wants to open your editor (if you have one)
    if (FIDStringsEqual(name, Vst::ViewType::kEditor))
    {
        // Create VST3-specific editor that wraps the WebView
        return new SingularityVST3Editor(this);
    }
    return nullptr;
}

//------------------------------------------------------------------------
} // namespace MyCompanyName
