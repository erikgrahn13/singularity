//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#include "vst3processor.h"
#include "plugincids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include <span>

using namespace Steinberg;

namespace {
struct VST3ParamChanges : IParameterChanges {
    Vst::IParameterChanges* src = nullptr;

    static bool isInternal(Vst::ParamID id) {
        return id == (Vst::ParamID)std::numeric_limits<int>::max(); // bypass
    }

    int getCount() const override {
        if (!src) return 0;
        int count = 0;
        for (int i = 0; i < src->getParameterCount(); ++i) {
            auto* q = src->getParameterData(i);
            if (q && q->getPointCount() > 0 && !isInternal(q->getParameterId())) ++count;
        }
        return count;
    }
    ParameterChange get(int index) const override {
        if (!src) return {};
        int found = 0;
        for (int i = 0; i < src->getParameterCount(); ++i) {
            auto* q = src->getParameterData(i);
            if (!q || q->getPointCount() == 0 || isInternal(q->getParameterId())) continue;
            if (found++ == index) {
                Vst::ParamValue val = 0; int32 offset = 0;
                q->getPoint(0, offset, val);
                return { (int)q->getParameterId(), val };
            }
        }
        return {};
    }
};
} // namespace

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
			if (paramQueue && paramQueue->getParameterId() == std::numeric_limits<int>::max())
			{
				Vst::ParamValue value{0};
				int32 sampleOffset{0};
				int32 numPoints = paramQueue->getPointCount ();

				paramQueue->getPoint(paramQueue->getPointCount() - 1, sampleOffset, value);
				mBypass = (value >= 0.5);
			}
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
        VST3ParamChanges paramChanges;
        paramChanges.src = data.inputParameterChanges;

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
				paramChanges);
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

	// called when we load a preset or project, the model has to be reloaded

	IBStreamer streamer (state, kLittleEndian);

	float savedParam1 = 0.f;
	if (streamer.readFloat (savedParam1) == false)
		return kResultFalse;

	int32 savedParam2 = 0;
	if (streamer.readInt32 (savedParam2) == false)
		return kResultFalse;

	int32 savedBypass = 0;
	if (streamer.readInt32 (savedBypass) == false)
		return kResultFalse;

	mParam1 = savedParam1;
	mParam2 = savedParam2 > 0 ? 1 : 0;
	mBypass = savedBypass > 0;

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Processor::getState (IBStream* state)
{
	// here we need to save the model (preset or project)

	float toSaveParam1 = mParam1;
	int32 toSaveParam2 = mParam2;
	int32 toSaveBypass = mBypass ? 1 : 0;

	IBStreamer streamer (state, kLittleEndian);
	streamer.writeFloat (toSaveParam1);
	streamer.writeInt32 (toSaveParam2);
	streamer.writeInt32 (toSaveBypass);

	return kResultOk;
}





//------------------------------------------------------------------------
} // namespace Steinberg