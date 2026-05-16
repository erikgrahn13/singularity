#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vstbypassprocessor.h"
#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "plugincids.h"
#include "../SingularityPlugin.h"
#include <span>
#include <map>
#include <limits>
#include <cstring>

namespace Steinberg {

template<::SingularityPlugin PluginType>
class VST3Processor : public Steinberg::Vst::AudioEffect
{
public:
	VST3Processor () { setControllerClass (kVST3ControllerUID); }
	~VST3Processor () override = default;

	static Steinberg::FUnknown* createInstance (void* /*context*/)
	{
		return (Steinberg::Vst::IAudioProcessor*)new VST3Processor;
	}

	tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE
	{
		tresult result = AudioEffect::initialize (context);
		if (result != kResultOk) return result;
		addAudioInput  (STR16 ("Stereo In"),  Vst::SpeakerArr::kStereo);
		addAudioOutput (STR16 ("Stereo Out"), Vst::SpeakerArr::kStereo);
		addEventInput  (STR16 ("Event In"), 1);
		return kResultOk;
	}

	tresult PLUGIN_API terminate () SMTG_OVERRIDE
	{
		return AudioEffect::terminate ();
	}

	tresult PLUGIN_API setActive (TBool state) SMTG_OVERRIDE
	{
		return AudioEffect::setActive (state);
	}

	tresult PLUGIN_API setupProcessing (Vst::ProcessSetup& newSetup) SMTG_OVERRIDE
	{
		mBypassProcessorFloat.setup  (*this, newSetup, getLatencySamples ());
		mBypassProcessorDouble.setup (*this, newSetup, getLatencySamples ());
		mPlugin.prepare(newSetup.sampleRate, newSetup.maxSamplesPerBlock);
		return AudioEffect::setupProcessing (newSetup);
	}

	tresult PLUGIN_API canProcessSampleSize (int32 symbolicSampleSize) SMTG_OVERRIDE
	{
		if (symbolicSampleSize == Vst::kSample32 || symbolicSampleSize == Vst::kSample64)
			return kResultTrue;
		return kResultFalse;
	}

	tresult PLUGIN_API process (Vst::ProcessData& data) SMTG_OVERRIDE
	{
		//--- Read inputs parameter changes-----------
		if (data.inputParameterChanges)
		{
			for (int i = 0; i < data.inputParameterChanges->getParameterCount (); ++i)
			{
				Vst::IParamValueQueue* paramQueue = data.inputParameterChanges->getParameterData (i);
				if (!paramQueue || paramQueue->getPointCount () == 0) continue;
				Vst::ParamValue value {0};
				int32 sampleOffset {0};
				paramQueue->getPoint (paramQueue->getPointCount () - 1, sampleOffset, value);
				if (paramQueue->getParameterId () == (Vst::ParamID)std::numeric_limits<int>::max ())
				{
					mBypassProcessorFloat.setActive  (value >= 0.5);
					mBypassProcessorDouble.setActive (value >= 0.5);
				}
				else
					mParamValues[(int)paramQueue->getParameterId ()] = value;
			}
		}

		//--- Process Audio---------------------
		if (data.numSamples > 0 && data.numInputs > 0 && data.numOutputs > 0)
		{
			int32 numChannels    = data.inputs[0].numChannels;
			auto  sampleFramesSz = getSampleFramesSizeInBytes (processSetup, data.numSamples);
			auto** in  = getChannelBuffersPointer (processSetup, data.inputs[0]);
			auto** out = getChannelBuffersPointer (processSetup, data.outputs[0]);
			int numIn  = data.inputs[0].numChannels;
			int numOut = data.outputs[0].numChannels;
			const bool host64 = data.symbolicSampleSize == Vst::kSample64;

			if (data.inputs[0].silenceFlags == Vst::getChannelMask (data.inputs[0].numChannels))
			{
				data.outputs[0].silenceFlags = data.inputs[0].silenceFlags;
				for (int32 i = 0; i < numChannels; i++)
					if (in[i] != out[i])
						std::memset (out[i], 0, sampleFramesSz);
			}
			else
			{
				data.outputs[0].silenceFlags = 0;
				if (mBypassProcessorFloat.isActive ())
				{
					if (host64) mBypassProcessorDouble.process (data);
					else        mBypassProcessorFloat.process  (data);
				}
				else if (host64)
				{
					auto** in64  = data.inputs[0].channelBuffers64;
					auto** out64 = data.outputs[0].channelBuffers64;
					mPlugin.template process<double> (
						std::span<const double* const> (in64,  numIn),
						std::span<double* const>       (out64, numOut),
						data.numSamples, mParamValues);
				}
				else
				{
					auto** in32  = data.inputs[0].channelBuffers32;
					auto** out32 = data.outputs[0].channelBuffers32;
					mPlugin.template process<float> (
						std::span<const float* const> (in32,  numIn),
						std::span<float* const>       (out32, numOut),
						data.numSamples, mParamValues);
				}
			}
		}
		return kResultOk;
	}

	tresult PLUGIN_API setState (IBStream* state) SMTG_OVERRIDE
	{
		if (!state) return kResultFalse;
		IBStreamer streamer (state, kLittleEndian);
		int32 savedBypass = 0;
		if (!streamer.readInt32 (savedBypass)) return kResultFalse;
		bool bypass = savedBypass > 0;
		mBypassProcessorFloat.setActive  (bypass);
		mBypassProcessorDouble.setActive (bypass);
		for (auto& [id, value] : mParamValues)
			if (!streamer.readDouble (value)) break;
		return kResultOk;
	}

	tresult PLUGIN_API getState (IBStream* state) SMTG_OVERRIDE
	{
		IBStreamer streamer (state, kLittleEndian);
		streamer.writeInt32 (mBypassProcessorFloat.isActive () ? 1 : 0);
		for (auto& [id, value] : mParamValues)
			streamer.writeDouble (value);
		return kResultOk;
	}

protected:
	PluginType mPlugin;
	Vst::BypassProcessor<Vst::Sample32> mBypassProcessorFloat;
	Vst::BypassProcessor<Vst::Sample64> mBypassProcessorDouble;
	std::map<int, double> mParamValues;
};

} // namespace Steinberg
