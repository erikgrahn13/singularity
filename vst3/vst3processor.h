#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "public.sdk/source/vst/vstbypassprocessor.h"
#include "public.sdk/source/vst/utility/audiobuffers.h"
#include "public.sdk/source/vst/utility/sampleaccurate.h"
#include "public.sdk/source/vst/utility/processdataslicer.h"
#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "public.sdk/source/vst/utility/dataexchange.h"
#include "AudioDataExchange.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstunits.h"
#include "plugincids.h"
#include PLUGIN_CLASS_HEADER
#include "SingularityPlugin.h"
#include "Vst3ComponentState.h"
#include "Vst3ParameterSupport.h"
#include "Vst3ProgramData.h"
#include "Vst3ProgramLayout.h"
#include "Vst3ProgramModel.h"
#include <atomic>
#include <span>
#include <vector>
#include <algorithm>
#include <cmath>
#include <limits>
#include <cstring>
#include <type_traits>
#include <memory>

namespace Steinberg {

template<::SingularityPlugin PluginType>
class VST3Processor : public Steinberg::Vst::AudioEffect,
                      public Steinberg::Vst::IProgramListData,
                      public Singularity::AudioDataExchange::IDataSink
{
public:
	VST3Processor () { setControllerClass (kVST3ControllerUID); }
	~VST3Processor () override = default;

	static Steinberg::FUnknown* createInstance (void* /*context*/)
	{
		return (Steinberg::Vst::IAudioProcessor*)new VST3Processor;
	}

	tresult PLUGIN_API queryInterface (const TUID iid, void** obj) SMTG_OVERRIDE
	{
		if constexpr (HasVst3ProgramLists<PluginType>)
		{
			QUERY_INTERFACE (
				iid, obj, getTUID<Vst::IProgramListData> (), Vst::IProgramListData)
		}
		return AudioEffect::queryInterface (iid, obj);
	}
	REFCOUNT_METHODS (AudioEffect)

		tresult PLUGIN_API initialize (FUnknown* context) SMTG_OVERRIDE
		{
			tresult result = AudioEffect::initialize (context);
			if (result != kResultOk) return result;
			resetPendingProcessorState();
			mBypassProcessorFloat.setActive(false);
			mBypassProcessorDouble.setActive(false);

			if constexpr (!PluginType::isInstrument)
				addAudioInput  (STR16 ("Stereo In"),  Vst::SpeakerArr::kStereo);
		addAudioOutput (STR16 ("Stereo Out"), Vst::SpeakerArr::kStereo);
		addEventInput  (STR16 ("Event In"), 1);

		mParams.clear();
		for (auto& parameter : PluginType::getParameters ())
		{
			const auto normalizedDefault =
				SingularityVst3::plainToNormalized(
					parameter, parameter.defaultValue);
			mParams.push_back ({parameter, { parameter.id, normalizedDefault}, normalizedDefault, 0.0});
		}
		initializePublishedProcessorState();
		if (!initializeProgramBanks())
			return kInvalidArgument;
		return kResultOk;
	}

		tresult PLUGIN_API terminate () SMTG_OVERRIDE
		{
			resetPendingProcessorState();
			return AudioEffect::terminate ();
		}

	tresult PLUGIN_API connect (Vst::IConnectionPoint* other) SMTG_OVERRIDE
	{
		auto result = AudioEffect::connect(other);
		if (result == kResultTrue)
		{
			auto configCallback = [] (Vst::DataExchangeHandler::Config& config, const Vst::ProcessSetup&) {
				config.blockSize = sizeof(Singularity::AudioDataExchange::AudioDataBlock);
				config.numBlocks = 8;
				config.alignment = 32;
				config.userContextID = Singularity::AudioDataExchange::kDefaultContextID;
				return true;
			};
			mDataExchange = std::make_unique<Vst::DataExchangeHandler>(this, configCallback);
			mDataExchange->onConnect(other, getHostContext());
		}
		return result;
	}

	tresult PLUGIN_API disconnect (Vst::IConnectionPoint* other) SMTG_OVERRIDE
	{
		if (mDataExchange)
		{
			mDataExchange->onDisconnect(other);
			mDataExchange.reset();
		}
		return AudioEffect::disconnect(other);
	}

	tresult PLUGIN_API setActive (TBool state) SMTG_OVERRIDE
	{
		if (mDataExchange)
		{
			if (state) mDataExchange->onActivate(processSetup);
			else mDataExchange->onDeactivate();
		}
		return AudioEffect::setActive (state);
	}

	tresult PLUGIN_API setupProcessing (Vst::ProcessSetup& newSetup) SMTG_OVERRIDE
	{
		mBypassProcessorFloat.setup  (*this, newSetup, getLatencySamples ());
		mBypassProcessorDouble.setup (*this, newSetup, getLatencySamples ());
		mPlugin.prepare (newSetup.sampleRate, newSetup.maxSamplesPerBlock);
		applyPendingProcessorState();
		for (auto& bank : mProgramBanks)
		{
			const auto pending =
				bank->pendingProgram.exchange(
					RuntimeProgramBank::kNoPendingProgram,
					std::memory_order_acq_rel);
			const auto selected =
				pending == RuntimeProgramBank::kNoPendingProgram
				? bank->currentProgram.load(std::memory_order_acquire)
				: RuntimeProgramBank::programIndex(pending);
			const auto applyParameters =
				pending == RuntimeProgramBank::kNoPendingProgram ||
				RuntimeProgramBank::appliesParameters(pending);
			applyProgram(*bank, selected, applyParameters);
		}
		publishProcessorState();

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
		if (!mProgramBanks.empty())
		{
			Vst::Algo::foreach (inputParameterChanges, [&] (Vst::IParamValueQueue& queue)
			{
				auto* bank = findProgramBankBySelector(
					queue.getParameterId());
				if (!bank)
					return;

				Vst::ParamValue value = 0.0;
				int32 offset = 0;
				if (queue.getPointCount () > 0 &&
					queue.getPoint (queue.getPointCount () - 1, offset, value) ==
						kResultTrue)
					applyProgram(*bank, programIndex(*bank, value));
			});
		}

		Vst::Algo::foreach (inputParameterChanges, [&] (Vst::IParamValueQueue& queue)
		{
			Vst::ParamID paramID = queue.getParameterId ();
			if (findProgramBankBySelector(paramID))
				return;
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
					if (param.metadata.readOnly) continue;
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
		applyPendingProcessorState();
		for (auto& bank : mProgramBanks)
		{
			const auto pendingProgram =
				bank->pendingProgram.exchange(
					RuntimeProgramBank::kNoPendingProgram,
					std::memory_order_acq_rel);
			if (pendingProgram != RuntimeProgramBank::kNoPendingProgram)
			{
				applyProgram(
					*bank,
					RuntimeProgramBank::programIndex(pendingProgram),
					RuntimeProgramBank::appliesParameters(
						pendingProgram));
			}
		}

		// Output parameters are calculated afresh for each process block. This
		// also guarantees that host-declared silence publishes zero/default.
		for (auto& parameter : mParams)
		{
			if (!parameter.metadata.readOnly) continue;
			const auto normalizedDefault =
				SingularityVst3::plainToNormalized(
				parameter.metadata, parameter.metadata.defaultValue);
			parameter.smoothed = normalizedDefault;
			parameter.rampTarget = normalizedDefault;
		}

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

		publishOutputParameters(data);

		//--- Cleanup SA params ---
		for (auto& parameter : mParams)
			if (!parameter.metadata.readOnly) parameter.saParam.endChanges ();

		publishProcessorState();
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
				if (mParams[i].metadata.readOnly)
				{
					params[i] = {mParams[i].metadata.id,
						SingularityVst3::normalizedToPlain(
							mParams[i].metadata, mParams[i].smoothed)};
					continue;
				}
				double target = mParams[i].saParam.advance (slice.numSamples);
				if (mParams[i].metadata.type != ParamType::Float)
				{
					mParams[i].smoothed = target;
					params[i] = {
						mParams[i].saParam.getParamID(),
						SingularityVst3::normalizedToPlain(
							mParams[i].metadata, mParams[i].smoothed)};
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
				params[i] = {
					mParams[i].saParam.getParamID(),
					SingularityVst3::normalizedToPlain(
						mParams[i].metadata, mParams[i].smoothed)};
			}

			Vst::AudioBusBuffers* outputs = slice.outputs;
			auto outputBuffers = Vst::getChannelBuffers<SampleSize> (*outputs);

			auto outputSpan = std::span<SampleT* const>(outputBuffers, outputs->numChannels);
			if constexpr (PluginType::isInstrument)
			{
				auto midiSpan = std::span<const MidiEvent>(mMidiEvents);
				processInstrumentPlugin<SampleT>(outputSpan, slice.numSamples, midiSpan, ParamList{params});
			}
			else
			{
				Vst::AudioBusBuffers* inputs = slice.inputs;
				auto inputBuffers = Vst::getChannelBuffers<SampleSize> (*inputs);
				auto inputSpan = std::span<const SampleT* const>(inputBuffers, inputs->numChannels);
				processEffectPlugin<SampleT>(inputSpan, outputSpan, slice.numSamples, ParamList{params});
			}

			for (int i = 0; i < static_cast<int>(mParams.size()); ++i)
			{
				if (!mParams[i].metadata.readOnly) continue;
				mParams[i].smoothed =
					SingularityVst3::plainToNormalized(
						mParams[i].metadata, params[i].second);
				mParams[i].rampTarget = mParams[i].smoothed;
			}
		};

		Vst::ProcessDataSlicer slicer (16);
		slicer.process<SampleSize> (data, doProcessing);
	}

	template<typename SampleT>
	void processInstrumentPlugin(std::span<SampleT* const> outputs, int numSamples, std::span<const MidiEvent> midiEvents, ParamList params)
	{
		Singularity::AudioDataExchange::ScopedSendContext sendContext(this, processSetup.sampleRate);
		mPlugin.template process<SampleT>(outputs, numSamples, midiEvents, params);
	}

	template<typename SampleT>
	void processEffectPlugin(std::span<const SampleT* const> inputs, std::span<SampleT* const> outputs, int numSamples, ParamList params)
	{
		Singularity::AudioDataExchange::ScopedSendContext sendContext(this, processSetup.sampleRate);
		mPlugin.template process<SampleT>(inputs, outputs, numSamples, params);
	}

	void pushAudioDataBlock(const Singularity::AudioDataExchange::AudioDataBlock& block) override
	{
		if (!mDataExchange)
			return;

		auto exchangeBlock = mDataExchange->getCurrentOrNewBlock();
		if (exchangeBlock.blockID != Vst::InvalidDataExchangeBlockID && exchangeBlock.data)
		{
			*Singularity::AudioDataExchange::toAudioDataBlock(exchangeBlock.data) = block;
			mDataExchange->sendCurrentBlock();
		}
	}

	tresult PLUGIN_API setState (IBStream* state) SMTG_OVERRIDE
	{
		const auto pluginParameters = PluginType::getParameters();
		SingularityVst3::ComponentState restored;
		if (!SingularityVst3::readComponentState(
				state, pluginParameters, restored))
			return kResultFalse;

		for (auto& bank : mProgramBanks)
		{
			for (auto& program : bank->programs)
				program->resetToFactory();
			bank->currentProgram.store(0, std::memory_order_release);
			bank->pendingProgram.store(0, std::memory_order_release);
		}

		for (auto& saved : restored.modifiedPrograms)
		{
			auto* bank = findProgramBank(saved.listId);
			if (!bank || saved.programIndex < 0 ||
				saved.programIndex >=
					static_cast<int32>(bank->programs.size()))
				continue;

			auto& program =
				*bank->programs[
					static_cast<std::size_t>(saved.programIndex)];
			const auto* current = program.snapshotForUi();
			if (!current)
				return kResultFalse;
			auto updated =
				std::make_unique<SingularityVst3::ProgramData>(*current);
			for (const auto& [id, value] : saved.data.parameters)
			{
				const auto parameter = std::find_if(
					updated->parameters.begin(),
					updated->parameters.end(),
					[id](const auto& candidate)
					{
						return candidate.first == id;
					});
				if (parameter == updated->parameters.end())
					continue;
				parameter->second = value;
			}
			updated->payload = std::move(saved.data.payload);
			program.replaceSnapshot(std::move(updated), true);
		}

		for (const auto& selection : restored.programSelections)
		{
			auto* bank = selection.listId == Vst::kNoProgramListId
				? (mProgramBanks.empty() ? nullptr : mProgramBanks.front().get())
				: findProgramBank(selection.listId);
			if (!bank || selection.programIndex < 0 ||
				selection.programIndex >=
					static_cast<int32>(bank->programs.size()))
				continue;
			bank->currentProgram.store(
				selection.programIndex, std::memory_order_release);
			bank->pendingProgram.store(
				selection.programIndex, std::memory_order_release);
		}

		auto pending = std::make_unique<PendingProcessorState>();
		pending->bypass = restored.bypass;
		pending->parameterValues = std::move(restored.parameterValues);
		const auto* pendingState = queueProcessorState(std::move(pending));
		publishRestoredProcessorState(*pendingState);
		return kResultOk;
	}

	tresult PLUGIN_API getState (IBStream* state) SMTG_OVERRIDE
	{
		SingularityVst3::ComponentState current;
		{
			PublishedStateUiGuard guard(mPublishedProcessorStateLock);
			current.bypass = mPublishedProcessorState.bypass;
			current.parameterValues =
				mPublishedProcessorState.parameterValues;
		}
		for (const auto& bank : mProgramBanks)
		{
			current.programSelections.push_back({
				bank->listId,
				bank->currentProgram.load(std::memory_order_relaxed),
			});
			for (std::size_t index = 0;
				 index < bank->programs.size();
				 ++index)
			{
				const auto& program = *bank->programs[index];
				if (!program.modified)
					continue;
				const auto* snapshot = program.snapshotForUi();
				if (!snapshot)
					return kResultFalse;
				current.modifiedPrograms.push_back({
					bank->listId,
					static_cast<int32>(index),
					*snapshot,
				});
			}
		}

		const auto pluginParameters = PluginType::getParameters();
		return SingularityVst3::writeComponentState(
			state, pluginParameters, current)
			? kResultOk
			: kResultFalse;
	}

	tresult PLUGIN_API programDataSupported (
		Vst::ProgramListID listId) SMTG_OVERRIDE
	{
		return findProgramBank(listId) ? kResultTrue : kResultFalse;
	}

	tresult PLUGIN_API getProgramData (
		Vst::ProgramListID listId,
		int32 programIndex,
		IBStream* data) SMTG_OVERRIDE
	{
		auto* bank = findProgramBank(listId);
		if (!bank || programIndex < 0 ||
			programIndex >= static_cast<int32>(bank->programs.size()) ||
			!data)
			return kInvalidArgument;

		const auto* snapshot =
			bank->programs[static_cast<std::size_t>(programIndex)]
				->snapshotForUi();
		return snapshot &&
			SingularityVst3::writeProgramData(data, *snapshot)
			? kResultTrue
			: kResultFalse;
	}

	tresult PLUGIN_API setProgramData (
		Vst::ProgramListID listId,
		int32 programIndex,
		IBStream* data) SMTG_OVERRIDE
	{
		auto* bank = findProgramBank(listId);
		if (!bank || programIndex < 0 ||
			programIndex >= static_cast<int32>(bank->programs.size()) ||
			!data)
			return kInvalidArgument;

		SingularityVst3::ProgramData decoded;
		if (!SingularityVst3::readProgramData(data, decoded))
			return kResultFalse;
		if constexpr (!HandlesProgramData<PluginType>)
			if (!decoded.payload.empty())
				return kInvalidArgument;

		auto& program =
			*bank->programs[static_cast<std::size_t>(programIndex)];
		const auto* current = program.snapshotForUi();
		if (!current)
			return kResultFalse;
		auto updated =
			std::make_unique<SingularityVst3::ProgramData>(*current);
		for (const auto& [id, value] : decoded.parameters)
		{
			auto found = false;
			for (auto& [updatedId, updatedValue] : updated->parameters)
			{
				if (updatedId == id)
				{
					updatedValue = value;
					found = true;
					break;
				}
			}
			if (!found)
				return kInvalidArgument;
		}
		updated->payload = std::move(decoded.payload);
		program.replaceSnapshot(std::move(updated), true);

		if (bank->currentProgram.load(std::memory_order_acquire) == programIndex)
		{
			bank->pendingProgram.store(
				RuntimeProgramBank::encodePendingProgram(
					programIndex, true),
				std::memory_order_release);
		}
		return kResultTrue;
	}

protected:
	struct PendingProcessorState
	{
		bool bypass = false;
		std::vector<SingularityVst3::SerializedParameter> parameterValues;
	};
	static_assert(
		std::atomic<const PendingProcessorState*>::is_always_lock_free);

	struct PublishedProcessorState
	{
		bool bypass = false;
		std::vector<SingularityVst3::SerializedParameter> parameterValues;
	};

	class PublishedStateUiGuard
	{
	public:
		explicit PublishedStateUiGuard(std::atomic_flag& lock)
			: lock_(lock)
		{
			while (lock_.test_and_set(std::memory_order_acquire))
			{
			}
		}

		~PublishedStateUiGuard()
		{
			lock_.clear(std::memory_order_release);
		}

	private:
		std::atomic_flag& lock_;
	};

		void initializePublishedProcessorState()
		{
		mPublishedProcessorState.bypass = false;
		mPublishedProcessorState.parameterValues.clear();
		mPublishedProcessorState.parameterValues.reserve(mParams.size());
		for (const auto& parameter : mParams)
		{
			if (!parameter.metadata.readOnly)
				mPublishedProcessorState.parameterValues.emplace_back(
					parameter.metadata.id, parameter.smoothed);
			}
		}

		void resetPendingProcessorState()
		{
			mPendingProcessorState.store(nullptr, std::memory_order_release);
			mProcessorStateHazard.store(nullptr, std::memory_order_release);
			mOwnedProcessorStates.clear();
		}

		const PendingProcessorState* queueProcessorState(
		std::unique_ptr<const PendingProcessorState> state)
	{
		const auto* active = state.get();
		mOwnedProcessorStates.push_back(std::move(state));
		mPendingProcessorState.store(active);

		const auto* hazard = mProcessorStateHazard.load();
		std::erase_if(
			mOwnedProcessorStates,
			[active, hazard](const auto& candidate)
			{
				return candidate.get() != active &&
					candidate.get() != hazard;
			});
		return active;
	}

	void applyPendingProcessorState()
	{
		const PendingProcessorState* pending = nullptr;
		for (;;)
		{
			pending = mPendingProcessorState.load();
			mProcessorStateHazard.store(pending);
			if (pending != mPendingProcessorState.load())
				continue;
			if (!pending ||
				mPendingProcessorState.compare_exchange_strong(
					pending, nullptr))
				break;
		}
		if (pending)
		{
			mBypassProcessorFloat.setActive(pending->bypass);
			mBypassProcessorDouble.setActive(pending->bypass);
			for (const auto& [id, value] : pending->parameterValues)
			{
				for (auto& parameter : mParams)
				{
					if (parameter.metadata.id != id ||
						parameter.metadata.readOnly)
						continue;
					parameter.smoothed = value;
					parameter.rampTarget = value;
					parameter.rampPerStep = 0.0;
					parameter.saParam.setValue(value);
					break;
				}
			}
		}
		mProcessorStateHazard.store(nullptr);
	}

	void publishRestoredProcessorState(
		const PendingProcessorState& restored)
	{
		PublishedStateUiGuard guard(mPublishedProcessorStateLock);
		mPublishedProcessorState.bypass = restored.bypass;
		for (const auto& [id, value] : restored.parameterValues)
		{
			for (auto& [publishedId, publishedValue] :
				 mPublishedProcessorState.parameterValues)
			{
				if (publishedId == id)
				{
					publishedValue = value;
					break;
				}
			}
		}
	}

	void publishProcessorState()
	{
		if (mPendingProcessorState.load(std::memory_order_acquire) ||
			mPublishedProcessorStateLock.test_and_set(
				std::memory_order_acquire))
			return;
		if (mPendingProcessorState.load(std::memory_order_acquire))
		{
			mPublishedProcessorStateLock.clear(std::memory_order_release);
			return;
		}

		mPublishedProcessorState.bypass =
			mBypassProcessorFloat.isActive();
		for (std::size_t index = 0; index < mParams.size(); ++index)
		{
			const auto& parameter = mParams[index];
			if (parameter.metadata.readOnly)
				continue;
			for (auto& [id, value] :
				 mPublishedProcessorState.parameterValues)
			{
				if (id == parameter.metadata.id)
				{
					value = parameter.smoothed;
					break;
				}
			}
		}
		mPublishedProcessorStateLock.clear(std::memory_order_release);
	}

	void publishOutputParameters(Vst::ProcessData& data)
	{
		if (!data.outputParameterChanges)
			return;

		for (auto& parameter : mParams)
		{
			if (!parameter.metadata.readOnly) continue;

			int32 queueIndex = 0;
			auto* queue = data.outputParameterChanges->addParameterData(
				parameter.metadata.id, queueIndex);
			if (!queue) continue;

			int32 pointIndex = 0;
			queue->addPoint(std::max<int32>(0, data.numSamples - 1),
				parameter.smoothed, pointIndex);
		}
	}

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

	struct RuntimeProgram
	{
		static_assert(
			std::atomic<const SingularityVst3::ProgramData*>::
				is_always_lock_free);

		std::vector<std::unique_ptr<const SingularityVst3::ProgramData>>
			ownedSnapshots;
		const SingularityVst3::ProgramData* factorySnapshot = nullptr;
		std::atomic<const SingularityVst3::ProgramData*> activeSnapshot {
			nullptr};
		std::atomic<const SingularityVst3::ProgramData*> snapshotHazard {
			nullptr};
		bool modified = false;

		const SingularityVst3::ProgramData* acquireSnapshot()
		{
			const SingularityVst3::ProgramData* snapshot = nullptr;
			do
			{
				snapshot = activeSnapshot.load();
				snapshotHazard.store(snapshot);
			}
			while (snapshot != activeSnapshot.load());
			return snapshot;
		}

		void releaseSnapshot()
		{
			snapshotHazard.store(nullptr);
		}

		const SingularityVst3::ProgramData* snapshotForUi() const
		{
			return activeSnapshot.load(std::memory_order_acquire);
		}

		void setFactorySnapshot(
			std::unique_ptr<const SingularityVst3::ProgramData> snapshot)
		{
			factorySnapshot = snapshot.get();
			ownedSnapshots.push_back(std::move(snapshot));
			activeSnapshot.store(factorySnapshot, std::memory_order_release);
			modified = false;
		}

		void resetToFactory()
		{
			activeSnapshot.store(factorySnapshot, std::memory_order_release);
			modified = false;

			const auto* hazard =
				snapshotHazard.load(std::memory_order_acquire);
			std::erase_if(
				ownedSnapshots,
				[this, hazard](const auto& candidate)
				{
					return candidate.get() != factorySnapshot &&
						candidate.get() != hazard;
				});
		}

		void replaceSnapshot(
			std::unique_ptr<const SingularityVst3::ProgramData> snapshot,
			bool markModified)
		{
			const auto* active = snapshot.get();
			ownedSnapshots.push_back(std::move(snapshot));
			activeSnapshot.store(active);
			modified = modified || markModified;

			const auto* hazard = snapshotHazard.load();
			std::erase_if(
				ownedSnapshots,
				[this, active, hazard](const auto& candidate)
				{
					return candidate.get() != active &&
						candidate.get() != factorySnapshot &&
						candidate.get() != hazard;
				});
		}
	};

	struct RuntimeProgramBank
	{
		static constexpr int32 kNoPendingProgram = -1;

		static int32 encodePendingProgram(
			int32 index,
			bool applyParameters)
		{
			return applyParameters ? -index - 2 : index;
		}

		static int32 programIndex(int32 pending)
		{
			return pending < kNoPendingProgram ? -pending - 2 : pending;
		}

		static bool appliesParameters(int32 pending)
		{
			return pending < kNoPendingProgram;
		}

		::ProgramCollection definition;
		int32 unitId = Vst::kRootUnitId;
		Vst::ProgramListID listId = Vst::kNoProgramListId;
		Vst::ParamID selectorId = Vst::kNoParamId;
		std::vector<std::unique_ptr<RuntimeProgram>> programs;
		std::atomic<int32> currentProgram {0};
		std::atomic<int32> pendingProgram {kNoPendingProgram};
	};

	bool initializeProgramBanks()
	{
		mProgramBanks.clear();
		auto model = SingularityVst3::buildProgramModel<PluginType>();
		if (!model)
			return false;

		for (auto& definition : model->banks)
		{
			auto bank = std::make_unique<RuntimeProgramBank>();
			bank->definition = std::move(definition.definition);
			bank->unitId = definition.unitId;
			bank->listId = definition.listId;
			bank->selectorId = definition.selectorId;
			for (auto& data : definition.programs)
			{
				auto runtimeProgram = std::make_unique<RuntimeProgram>();
				runtimeProgram->setFactorySnapshot(
					std::make_unique<SingularityVst3::ProgramData>(
						std::move(data)));
				bank->programs.push_back(std::move(runtimeProgram));
			}
			mProgramBanks.push_back(std::move(bank));
		}
		return true;
	}

	int32 programIndex(
		const RuntimeProgramBank& bank,
		Vst::ParamValue normalizedValue) const
	{
		if (bank.programs.empty())
			return -1;
		const auto maxIndex = static_cast<double>(bank.programs.size() - 1);
		return static_cast<int32>(std::clamp(
			std::round(std::clamp(normalizedValue, 0.0, 1.0) * maxIndex),
			0.0,
			maxIndex));
	}

	bool applyProgram(
		RuntimeProgramBank& bank,
		int32 index,
		bool applyParameters = true)
	{
		if (index < 0 ||
			index >= static_cast<int32>(bank.programs.size()))
			return false;

		auto& program =
			*bank.programs[static_cast<std::size_t>(index)];
		const auto* snapshot = program.acquireSnapshot();
		if (!snapshot)
		{
			program.releaseSnapshot();
			return false;
		}
		if (applyParameters)
		{
			for (const auto& [id, value] : snapshot->parameters)
			{
				for (auto& parameter : mParams)
				{
					if (parameter.metadata.id != id ||
						parameter.metadata.readOnly)
						continue;
					parameter.smoothed = value;
					parameter.rampTarget = value;
					parameter.rampPerStep = 0.0;
					parameter.saParam.setValue(value);
					break;
				}
			}
		}

		if constexpr (HandlesProgramData<PluginType>)
		{
			const auto& program =
				bank.definition.programs[static_cast<std::size_t>(index)];
			mPlugin.loadProgramData(
				bank.definition.id,
				program.id,
				std::span<const std::byte>(
					snapshot->payload.data(), snapshot->payload.size()));
		}

		program.releaseSnapshot();
		bank.currentProgram.store(index, std::memory_order_release);
		return true;
	}

	RuntimeProgramBank* findProgramBank(Vst::ProgramListID listId)
	{
		for (auto& bank : mProgramBanks)
			if (bank->listId == listId)
				return bank.get();
		return nullptr;
	}

	const RuntimeProgramBank* findProgramBank(
		Vst::ProgramListID listId) const
	{
		for (const auto& bank : mProgramBanks)
			if (bank->listId == listId)
				return bank.get();
		return nullptr;
	}

	RuntimeProgramBank* findProgramBankBySelector(Vst::ParamID selectorId)
	{
		for (auto& bank : mProgramBanks)
			if (bank->selectorId == selectorId)
				return bank.get();
		return nullptr;
	}

	std::vector<Param> mParams;
	std::vector<std::unique_ptr<RuntimeProgramBank>> mProgramBanks;
	std::vector<MidiEvent> mMidiEvents;
	std::unique_ptr<Vst::DataExchangeHandler> mDataExchange;
	std::vector<std::unique_ptr<const PendingProcessorState>>
		mOwnedProcessorStates;
	std::atomic<const PendingProcessorState*> mPendingProcessorState {
		nullptr};
	std::atomic<const PendingProcessorState*> mProcessorStateHazard {
		nullptr};
	std::atomic_flag mPublishedProcessorStateLock = ATOMIC_FLAG_INIT;
	PublishedProcessorState mPublishedProcessorState;
	int mSmoothSteps = 0;
};

} // namespace Steinberg
