#include "vst3controller.h"
#include "plugincids.h"
#include "SingularityView.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "SingularityPlugin.h"
#include PLUGIN_CLASS_HEADER
#include <limits>
#include <memory>

using namespace Steinberg;

namespace Steinberg {

//------------------------------------------------------------------------
// VST3Controller Implpementation
//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::initialize (FUnknown* context)
{
	//---do not forget to call parent ------
	tresult result = EditControllerEx1::initialize (context);
	if (result != kResultOk)
	{
		return result;
	}

	parameters.addParameter (STR16 ("Bypass"), nullptr, 1, 0,
							Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass,
							Steinberg::Vst::kMaxParamId);

	for (auto& p : PLUGIN_CLASS::getParameters ())
		addSingularityParameter (p.id, p.name.c_str (), p.defaultValue);


	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::terminate ()
{
	// Here the Plug-in will be de-instantiated, last possibility to remove some memory!

	//---do not forget to call parent ------
	return EditControllerEx1::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::setComponentState (IBStream* state)
{
	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);

	// Read bypass first (written as int32 by processor)
	int32 bypassState = 0;
	if (!streamer.readInt32(bypassState)) return kResultFalse;
	setParamNormalized(Steinberg::Vst::kMaxParamId, bypassState ? 1 : 0);

	// Read remaining plugin parameters in the same order the processor wrote them
	for (int32 i = 0; i < parameters.getParameterCount(); ++i) {
		auto* param = parameters.getParameterByIndex(i);
		if (!param) continue;
		if (param->getInfo().id == Steinberg::Vst::kMaxParamId) continue; // bypass already read
		double value = 0.0;
		if (!streamer.readDouble(value)) break;
		setParamNormalized(param->getInfo().id, value);
	}

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::setState (IBStream* state)
{
	// Here you get the state of the controller

	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::getState (IBStream* state)
{
	// Here you are asked to deliver the state of the controller (if needed)
	// Note: the real state of your plug-in is saved in the processor

	return kResultTrue;
}

//------------------------------------------------------------------------
IPlugView* PLUGIN_API VST3Controller::createView (FIDString name)
{
	if (FIDStringsEqual (name, Vst::ViewType::kEditor))
	{
        // Default editor size — host may resize after creation
        return new SingularityView(this);
	}
	return nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::setParamNormalized (Vst::ParamID tag, Vst::ParamValue value)
{
	// called by host to update your parameters
	tresult result = EditControllerEx1::setParamNormalized (tag, value);
	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::getParamStringByValue (Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string)
{
	// called by host to get a string for given normalized value of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamStringByValue (tag, valueNormalized, string);
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::getParamValueByString (Vst::ParamID tag, Vst::TChar* string, Vst::ParamValue& valueNormalized)
{
	// called by host to get a normalized value from a string representation of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamValueByString (tag, string, valueNormalized);
}

//------------------------------------------------------------------------
} // namespace Steinberg