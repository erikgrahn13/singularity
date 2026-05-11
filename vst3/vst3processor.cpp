//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#include "vst3processor.h"
#include "plugincids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "../SingularityPlugin.h"
#include <span>
#include <limits>

using namespace Steinberg;

namespace Steinberg {
//------------------------------------------------------------------------
// VST3Processor
//------------------------------------------------------------------------
VST3Processor::VST3Processor ()
{
	setControllerClass (kVST3ControllerUID);
	mPlugin = createPlugin();
}

//------------------------------------------------------------------------
VST3Processor::~VST3Processor ()
{}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Processor::initialize (FUnknown* context)
{
	// Here the Plug-in will be instantiated
	
	//---always initialize the parent-------
	tresult result = AudioEffect::initialize (context);
	// if everything Ok, continue
	if (result != kResultOk)
	{
		return result;
	}

	//--- create Audio IO ------
	addAudioInput (STR16 ("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
	addAudioOutput (STR16 ("Stereo Out"), Steinberg::Vst::SpeakerArr::kStereo);

	/* If you don't need an event bus, you can remove the next line */
	addEventInput (STR16 ("Event In"), 1);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Processor::terminate ()
{
	// Here the Plug-in will be de-instantiated, last possibility to remove some memory!
	
	//---do not forget to call parent ------
	return AudioEffect::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Processor::setActive (TBool state)
{
	//--- called when the Plug-in is enable/disable (On/Off) -----
	return AudioEffect::setActive (state);
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Processor::process (Vst::ProcessData& data)
{
    //--- Read inputs parameter changes-----------
	if (data.inputParameterChanges)
	{
		for (int i = 0; i < data.inputParameterChanges->getParameterCount(); ++i)
		{
			Vst::IParamValueQueue* paramQueue = data.inputParameterChanges->getParameterData(i);
			if (!paramQueue || paramQueue->getPointCount() == 0) continue;
			Vst::ParamValue value{0};
			int32 sampleOffset{0};
			paramQueue->getPoint(paramQueue->getPointCount() - 1, sampleOffset, value);
			if (paramQueue->getParameterId() == (Vst::ParamID)std::numeric_limits<int>::max())
				mBypass = (value >= 0.5);
			else
				mParamValues[(int)paramQueue->getParameterId()] = value;
		}
	}
    //--- Process Audio---------------------
    if (data.numSamples > 0 && mPlugin
        && data.numInputs > 0 && data.numOutputs > 0
        && data.inputs[0].channelBuffers32 && data.outputs[0].channelBuffers32)
    {
        // Build span views over the DAW's channel buffers (float32 only)
        auto** in  = data.inputs[0].channelBuffers32;
        auto** out = data.outputs[0].channelBuffers32;
        int    numIn  = data.inputs[0].numChannels;
        int    numOut = data.outputs[0].numChannels;

        // Wrap VST3 parameter changes into IParameterChanges
        // (mParamValues is passed directly to plugin)

		if(mBypass)
		{
			// Pass-through: copy input → output
             auto** in  = data.inputs[0].channelBuffers32;
             auto** out = data.outputs[0].channelBuffers32;
             int numCh = std::min(data.inputs[0].numChannels, data.outputs[0].numChannels);
             for (int ch = 0; ch < numCh; ++ch)
                 std::memcpy(out[ch], in[ch], data.numSamples * sizeof(float));
		}
		else
		{
			mPlugin->process(
				std::span<const float* const>(reinterpret_cast<const float* const*>(in),  numIn),
				std::span<float* const>(out, numOut),
				data.numSamples,
				mParamValues);
		}
    }
    return kResultOk;

}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Processor::setupProcessing (Vst::ProcessSetup& newSetup)
{
	//--- called before any processing ----
	return AudioEffect::setupProcessing (newSetup);
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Processor::canProcessSampleSize (int32 symbolicSampleSize)
{
	// by default kSample32 is supported
	if (symbolicSampleSize == Vst::kSample32)
		return kResultTrue;

	// disable the following comment if your processing support kSample64
	/* if (symbolicSampleSize == Vst::kSample64)
		return kResultTrue; */

	return kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Processor::setState (IBStream* state)
{
	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);
	int32 savedBypass = 0;
	if (!streamer.readInt32(savedBypass)) return kResultFalse;
	mBypass = savedBypass > 0;

	for (auto& [id, value] : mParamValues)
		if (!streamer.readDouble(value)) break;

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Processor::getState (IBStream* state)
{
	IBStreamer streamer (state, kLittleEndian);
	streamer.writeInt32(mBypass ? 1 : 0);
	for (auto& [id, value] : mParamValues)
		streamer.writeDouble(value);
	return kResultOk;
}





//------------------------------------------------------------------------
} // namespace Steinberg