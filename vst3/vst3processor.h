#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vstbypassprocessor.h"
#include "public.sdk/source/vst/utility/audiobuffers.h"
#include "public.sdk/source/vst/utility/sampleaccurate.h"
#include "public.sdk/source/vst/utility/processdataslicer.h"
#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "plugincids.h"
#include PLUGIN_CLASS_HEADER
#include "SingularityPlugin.h"
#include <span>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>
#include <cstring>
#include <type_traits>

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
		
		if constexpr (!PluginType::isInstrument)
			addAudioInput  (STR16 ("Stereo In"),  Vst::SpeakerArr::kStereo);
		addAudioOutput (STR16 ("Stereo Out"), Vst::SpeakerArr::kStereo);
		addEventInput  (STR16 ("Event In"), 1);

		mParams.clear();
		for (auto& parameter : PluginType::getParameters ())
		{
			const auto normalizedDefault = plainToNormalized(parameter, parameter.defaultValue);
			mParams.push_back ({parameter, { parameter.id, normalizedDefault}, normalizedDefault, 0.0});
		}		
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
		mPlugin.prepare (newSetup.sampleRate, newSetup.maxSamplesPerBlock);

		mMidiEvents.reserve (32);

		mSmoothSteps = static_cast<int> (newSetup.sampleRate * 0.005);
		if (mSmoothSteps < 1) 
			mSmoothSteps = 1;

		return AudioEffect::setupProcessing (newSetup);
	}

	tresult PLUGIN_API canProcessSampleSize (int32 symbolicSampleSize) SMTG_OVERRIDE
	{
		if (symbolicSampleSize == Vst::kSample32 || symbolicSampleSize == Vst::kSample64)
			return kResultTrue;
		return kResultFalse;
	}

	void handleParameterChanges (Vst::IParameterChanges* inputParameterChanges)
	{
		Vst::Algo::foreach (inputParameterChanges, [&] (Vst::IParamValueQueue& queue)
		{
			Vst::ParamID paramID = queue.getParameterId ();
			if (paramID == Steinberg::Vst::kMaxParamId) // Bypass parameter id
			{
				Vst::ParamValue value; 
				int32 offset;
				queue.getPoint (queue.getPointCount () - 1, offset, value);
				mBypassProcessorFloat.setActive  (value >= 0.5);
				mBypassProcessorDouble.setActive (value >= 0.5);
			}
			else 
			{
				for (auto& param : mParams)
				{
					if (param.saParam.getParamID () == paramID)
					{
						if (param.metadata.type == ParamType::Float)
							param.saParam.beginChanges (&queue);
						else
						{
							Vst::ParamValue value; int32 offset;
							queue.getPoint (queue.getPointCount () - 1, offset, value);
							param.saParam.setValue (value);
						}
						break;
					}
				}
			}
		});
	}

	tresult PLUGIN_API process (Vst::ProcessData& data) SMTG_OVERRIDE
	{
		handleParameterChanges (data.inputParameterChanges);


		// Collect MIDI events for this block
		mMidiEvents.clear ();
		if (data.inputEvents)
		{
			const int32 numEvents = data.inputEvents->getEventCount ();
			for (int32 i = 0; i < numEvents; ++i)
			{
				Vst::Event e;
				if (data.inputEvents->getEvent (i, e) != kResultTrue)
					continue;
				if (mMidiEvents.size () >= 32)
					break;
				switch (e.type)
				{
				case Vst::Event::kNoteOnEvent:
					mMidiEvents.push_back ({ MidiEvent::Type::NoteOn,  e.noteOn.pitch,  e.noteOn.velocity });
					break;
				case Vst::Event::kNoteOffEvent:
					mMidiEvents.push_back ({ MidiEvent::Type::NoteOff, e.noteOff.pitch, e.noteOff.velocity });
					break;
				case Vst::Event::kDataEvent:                  break;
				case Vst::Event::kPolyPressureEvent:          break;
				case Vst::Event::kNoteExpressionValueEvent:   break;
				case Vst::Event::kNoteExpressionTextEvent:    break;
				case Vst::Event::kChordEvent:                 break;
				case Vst::Event::kScaleEvent:                 break;
				case Vst::Event::kNoteExpressionIntValueEvent: break;
				case Vst::Event::kLegacyMIDICCOutEvent:       break;
				default:                                      break;
				}
			}
		}

		if (data.numSamples > 0 && (PluginType::isInstrument || data.numInputs > 0) && data.numOutputs > 0)
		{
			if (processSetup.symbolicSampleSize == Vst::kSample32)
				processAudio<Vst::kSample32> (data);
			else
				processAudio<Vst::kSample64> (data);
		}

		//--- Cleanup SA params ---
		for (auto& parameter : mParams)
			parameter.saParam.endChanges ();

		return kResultOk;
	}

	template <Vst::SymbolicSampleSizes SampleSize>
	void processAudio (Vst::ProcessData& data)
	{
		using SampleT = std::conditional_t<SampleSize == Vst::kSample32, float, double>;
		Vst::AudioBusBuffers* outputs = data.outputs;
		auto outputBuffers = Vst::getChannelBuffers<SampleSize> (*outputs);

		if constexpr (!PluginType::isInstrument)
		{
			Vst::AudioBusBuffers* inputs = data.inputs;
			auto inputBuffers  = Vst::getChannelBuffers<SampleSize> (*inputs);

			if (inputs->silenceFlags == Vst::getChannelMask (inputs->numChannels))
			{
				outputs->silenceFlags = inputs->silenceFlags;
				for (int i = 0; i < inputs->numChannels; ++i)
					if (inputBuffers[i] != outputBuffers[i])
						std::memset (outputBuffers[i], 0, data.numSamples * sizeof (SampleT));
				return;
			}

			if (mBypassProcessorFloat.isActive ())
			{
				if constexpr (SampleSize == Vst::kSample64) 
					mBypassProcessorDouble.process (data);
				else
					mBypassProcessorFloat.process  (data);
				
				outputs->silenceFlags = inputs->silenceFlags;
				return;
			}
		}
		
		outputs->silenceFlags = 0;

		auto doProcessing = [&] (Vst::ProcessData& slice)
		{
			std::array<std::pair<unsigned int, double>, std::tuple_size_v<decltype(PluginType::getParameters())>> params;
			for (int i = 0; i < mParams.size(); ++i)
			{
				double target = mParams[i].saParam.advance (slice.numSamples);
				if (mParams[i].metadata.type != ParamType::Float)
				{
					mParams[i].smoothed = target;
					params[i] = { mParams[i].saParam.getParamID(), normalizedToPlain(mParams[i].metadata, mParams[i].smoothed) };
					continue;
				}
				if (target != mParams[i].rampTarget)
				{
					mParams[i].rampPerStep = (target - mParams[i].smoothed) / (double)mSmoothSteps;
					mParams[i].rampTarget  = target;
				}
				if (mParams[i].rampPerStep != 0.0)
				{
					mParams[i].smoothed += mParams[i].rampPerStep * slice.numSamples;
					if ((mParams[i].rampPerStep > 0.0 && mParams[i].smoothed >= mParams[i].rampTarget) ||
					    (mParams[i].rampPerStep < 0.0 && mParams[i].smoothed <= mParams[i].rampTarget))
					{
						mParams[i].smoothed    = mParams[i].rampTarget;
						mParams[i].rampPerStep = 0.0;
					}
				}
				params[i] = { mParams[i].saParam.getParamID(), normalizedToPlain(mParams[i].metadata, mParams[i].smoothed) };
			}

			Vst::AudioBusBuffers* outputs = slice.outputs;
			auto outputBuffers = Vst::getChannelBuffers<SampleSize> (*outputs);

			auto outputSpan = std::span<SampleT* const>(outputBuffers, outputs->numChannels);
			if constexpr (PluginType::isInstrument)
			{
				auto midiSpan = std::span<const MidiEvent>(mMidiEvents);
				mPlugin.template process<SampleT> (outputSpan, slice.numSamples, midiSpan, ParamList{params});
			}
			else
			{
				Vst::AudioBusBuffers* inputs = slice.inputs;
				auto inputBuffers = Vst::getChannelBuffers<SampleSize> (*inputs);
				auto inputSpan = std::span<const SampleT* const>(inputBuffers, inputs->numChannels);
				mPlugin.template process<SampleT> (inputSpan, outputSpan, slice.numSamples, ParamList{params});
			}
		};

		Vst::ProcessDataSlicer slicer (16);
		slicer.process<SampleSize> (data, doProcessing);
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
		for (auto& parameter : mParams)
		{
			double value = 0.0;
			if (!streamer.readDouble (value)) break;
			parameter.smoothed   = value;
			parameter.rampTarget = value;
			parameter.saParam.setValue (value);
		}
		return kResultOk;
	}

	tresult PLUGIN_API getState (IBStream* state) SMTG_OVERRIDE
	{
		IBStreamer streamer (state, kLittleEndian);
		streamer.writeInt32 (mBypassProcessorFloat.isActive () ? 1 : 0);
		for (auto& parameter : mParams)
			streamer.writeDouble (parameter.smoothed);
		return kResultOk;
	}

protected:
	PluginType mPlugin;
	Vst::BypassProcessor<Vst::Sample32> mBypassProcessorFloat;
	Vst::BypassProcessor<Vst::Sample64> mBypassProcessorDouble;
	struct Param
	{
		::Parameter metadata;
		Vst::SampleAccurate::Parameter saParam;
		double smoothed    = 0.0;
		double rampTarget  = 0.0;
		double rampPerStep = 0.0;
	};

	static double plainToNormalized(const ::Parameter& parameter, double plainValue)
	{
		if (parameter.type == ParamType::Bool)
			return plainValue >= 0.5 ? 1.0 : 0.0;

		if (parameter.type == ParamType::Choice && !parameter.choices.empty())
		{
			const auto maxIndex = static_cast<double>(parameter.choices.size() - 1);
			if (maxIndex <= 0.0)
				return 0.0;
			return std::clamp(std::round(plainValue) / maxIndex, 0.0, 1.0);
		}

		if (parameter.maxValue == parameter.minValue)
			return 0.0;

		return std::clamp((plainValue - parameter.minValue) /
			(parameter.maxValue - parameter.minValue), 0.0, 1.0);
	}

	static double normalizedToPlain(const ::Parameter& parameter, double normalizedValue)
	{
		const auto clamped = std::clamp(normalizedValue, 0.0, 1.0);

		if (parameter.type == ParamType::Bool)
			return clamped >= 0.5 ? 1.0 : 0.0;

		if (parameter.type == ParamType::Choice && !parameter.choices.empty())
			return std::round(clamped * static_cast<double>(parameter.choices.size() - 1));

		const auto plain = parameter.minValue + clamped * (parameter.maxValue - parameter.minValue);
		if (parameter.type == ParamType::Stepped)
			return std::round(plain);

		return plain;
	}

	std::vector<Param> mParams;
	std::vector<MidiEvent> mMidiEvents;
	int mSmoothSteps = 0;
};

} // namespace Steinberg
