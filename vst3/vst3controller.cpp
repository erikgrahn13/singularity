#include "vst3controller.h"
#include "plugincids.h"
#include "SingularityView.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "../SingularityPlugin.h"
#include <memory>

using namespace Steinberg;

// VST3 implementation of createParameter — routes to the active controller
static HelloWorldController* g_controller = nullptr;

void createParameter(int id, const char* name, ParamType type,
                     double defaultValue, double minValue, double maxValue)
{
    if (!g_controller) return;
    g_controller->addSingularityParameter(id, name, defaultValue);
}

namespace Steinberg {

//------------------------------------------------------------------------
// HelloWorldController Implementation
//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldController::initialize (FUnknown* context)
{
	//---do not forget to call parent ------
	tresult result = EditControllerEx1::initialize (context);
	if (result != kResultOk)
	{
		return result;
	}

	g_controller = this;
	registerParameters();
	g_controller = nullptr;


	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldController::terminate ()
{
	// Here the Plug-in will be de-instantiated, last possibility to remove some memory!

	//---do not forget to call parent ------
	return EditControllerEx1::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldController::setComponentState (IBStream* state)
{
	// Here you get the state of the component (Processor part)
	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);

	// float savedParam1 = 0.f;
	// if (streamer.readFloat (savedParam1) == false)
	// 	return kResultFalse;
	// setParamNormalized (HelloWorldParams::kParamVolId, savedParam1);

	// int8 savedParam2 = 0;
	// if (streamer.readInt8 (savedParam2) == false)
	// 	return kResultFalse;
	// setParamNormalized (HelloWorldParams::kParamOnId, savedParam2);

	// // read the bypass
	// int32 bypassState;
	// if (streamer.readInt32 (bypassState) == false)
	// 	return kResultFalse;
	// setParamNormalized (kBypassId, bypassState ? 1 : 0);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldController::setState (IBStream* state)
{
	// Here you get the state of the controller

	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldController::getState (IBStream* state)
{
	// Here you are asked to deliver the state of the controller (if needed)
	// Note: the real state of your plug-in is saved in the processor

	return kResultTrue;
}

//------------------------------------------------------------------------
IPlugView* PLUGIN_API HelloWorldController::createView (FIDString name)
{
	if (FIDStringsEqual (name, Vst::ViewType::kEditor))
	{
        // Default editor size — host may resize after creation
        return new SingularityView(_paramAdapter);
	}
	return nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldController::setParamNormalized (Vst::ParamID tag, Vst::ParamValue value)
{
	// called by host to update your parameters
	tresult result = EditControllerEx1::setParamNormalized (tag, value);
	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldController::getParamStringByValue (Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string)
{
	// called by host to get a string for given normalized value of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamStringByValue (tag, valueNormalized, string);
}

//------------------------------------------------------------------------
tresult PLUGIN_API HelloWorldController::getParamValueByString (Vst::ParamID tag, Vst::TChar* string, Vst::ParamValue& valueNormalized)
{
	// called by host to get a normalized value from a string representation of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamValueByString (tag, string, valueNormalized);
}

//------------------------------------------------------------------------
} // namespace Steinberg