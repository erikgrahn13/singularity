//------------------------------------------------------------------------
// Copyright(c) 2026 My Plug-in Company.
//------------------------------------------------------------------------

#pragma once

#include <concepts>

#include "pluginterfaces/base/futils.h"
#include "public.sdk/source/vst/vstaudioeffect.h"
#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/vstbypassprocessor.h"
#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "public.sdk/source/vst/utility/sampleaccurate.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "public.sdk/source/vst/utility/audiobuffers.h"
#include "public.sdk/source/vst/utility/rttransfer.h"
#include "public.sdk/source/vst/utility/dataexchange.h"


#include "plugincids.h"
#include "SingularityPlugin.h"
#include "SingularityVST3ParameterFactory.h"
#include "dataexchange.h"

using namespace Steinberg;

static constexpr Steinberg::Vst::DataExchangeBlock InvalidDataExchangeBlock = {nullptr, 0,
                                                               Steinberg::Vst::InvalidDataExchangeBlockID};

//------------------------------------------------------------------------
//  VST3Processor
//------------------------------------------------------------------------
template<SingularityPlugin PluginType>
class VST3Processor : public Steinberg::Vst::AudioEffect
{
public:
    VST3Processor ()
    {
        setControllerClass(kVST3ControllerUID);

        for(const auto& definition : parameterDefinitions_)
        {
            auto* mapping = createVst3Parameter(definition);
            parameterMappings_.addParameter(mapping);

            if (definition.direction == ParameterDirection::input)
            {
                inputParameters_.emplace_back(definition.id, mapping->getInfo().defaultNormalizedValue);
            }
        }

        if constexpr (requires { plugin_.sendToUI.connect(*this); })
        {
            plugin_.sendToUI.connect(*this);
        }
    }

    ~VST3Processor () SMTG_OVERRIDE
    {}

    // Create function
    static Steinberg::FUnknown* createInstance (void* /*context*/) 
    { 
        return (Steinberg::Vst::IAudioProcessor*)new VST3Processor; 
    }

    //--- ---------------------------------------------------------------------
    // AudioEffect overrides:
    //--- ---------------------------------------------------------------------
    /** Called at first after constructor */
    Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE
    {
        tresult result = AudioEffect::initialize(context);
        if(result != kResultOk)
            return result;
        
        bypassProcessor32_.setActive(false);
        bypassProcessor64_.setActive(false);

        if(!PluginType::isInstrument)
            addAudioInput(STR16("Stereo In"), Vst::SpeakerArr::kStereo);
        addAudioOutput(STR16("Stereo Out"), Vst::SpeakerArr::kStereo);
        addEventInput(STR16("Event In"), 1);

        return kResultOk;
    }

    Steinberg::tresult PLUGIN_API connect (Vst::IConnectionPoint* other) SMTG_OVERRIDE
    {
        auto result = Vst::AudioEffect::connect(other);
        if(result == kResultTrue)
        {
            auto configCallback = [this](Vst::DataExchangeHandler::Config& config, const Vst::ProcessSetup& setup)
            {
                Vst::SpeakerArrangement arr;
                getBusArrangement(Vst::BusDirections::kInput, 0, arr);
                numChannels = static_cast<uint16_t>(Vst::SpeakerArr::getChannelCount(arr));
                auto sampleSize = sizeof(float);
                numSamplesToSend = setup.sampleRate / 60.0; // 60Hz

                // config.blockSize = sizeof(DataBlock) + (sizeof(float) * setup.sampleRate / 60) * 2;
                config.blockSize = sizeof(DataBlock) + sizeof(float) * numSamplesToSend * 2;
                config.numBlocks = 32;
                config.alignment = 32;
                config.userContextID = SampleBufferQueueID;
                return true;
            };

            // sizeof(DataBlock) + (sizeof(float) * samplerate / 60) * 2

            dataExchange = std::make_unique<Vst::DataExchangeHandler>(this, configCallback);
            dataExchange->onConnect(other, getHostContext());
        }
        return result;
    }

    Steinberg::tresult PLUGIN_API disconnect (Vst::IConnectionPoint* other) SMTG_OVERRIDE
    {
        if(dataExchange)
        {
            dataExchange->onDisconnect(other);
            dataExchange.reset();
        }
        return AudioEffect::disconnect(other);
    }

    
    /** Called at the end before destructor */
    Steinberg::tresult PLUGIN_API terminate () SMTG_OVERRIDE
    {
        stateTransfer.clear_ui();
        return AudioEffect::terminate();
    }
    
    /** Switch the Plug-in on/off */
    Steinberg::tresult PLUGIN_API setActive (Steinberg::TBool state) SMTG_OVERRIDE
    {
        if(state)
        {
            plugin_.prepare(processSetup.sampleRate, processSetup.maxSamplesPerBlock);
            bypassProcessor32_.setup(*this, processSetup, getLatencySamples());
            bypassProcessor64_.setup(*this, processSetup, getLatencySamples());
            dataExchange->onActivate(processSetup);
        }
        else
        {
            bypassProcessor32_.reset();
            bypassProcessor64_.reset();
            dataExchange->onDeactivate();
        }
        return AudioEffect::setActive(state);
    }

    /** Will be called before any process call */
    Steinberg::tresult PLUGIN_API setupProcessing (Steinberg::Vst::ProcessSetup& newSetup) SMTG_OVERRIDE
    {
        const auto result = AudioEffect::setupProcessing(newSetup);
        if(result != kResultOk)
            return result;

        for(auto& parameterBuffer : parameterBuffers_)
        {
            parameterBuffer.resize(static_cast<std::size_t>(newSetup.maxSamplesPerBlock));
        }

        return kResultOk;
    }
    
    /** Asks if a given sample size is supported see SymbolicSampleSizes. */
    Steinberg::tresult PLUGIN_API canProcessSampleSize (Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE
    {
        if(symbolicSampleSize == Vst::kSample32 || symbolicSampleSize == Vst::kSample64)
            return kResultTrue;
        return kResultFalse;
    }

    void handleParameterChanges(Vst::IParameterChanges* inputParameterChanges)
    {
        Vst::Algo::foreach(inputParameterChanges, [&](Vst::IParamValueQueue& queue)
        {
            if(queue.getParameterId() == bypassParameter_.getParamID())
            {
                bypassParameter_.beginChanges(&queue);
                return;
            }

            for(auto& parameter : inputParameters_)
            {
                if(parameter.getParamID() == queue.getParameterId())
                {
                    parameter.beginChanges(&queue);
                    return;
                }
            }
        });
    }

    void publishOutputParameterChanges(Vst::ProcessData& data)
    {
        if(data.outputParameterChanges)
        {
            for(int i = 0; i < parameterDefinitions_.size(); ++i)
            {
                if(parameterDefinitions_[i].direction != ParameterDirection::output || !outputParameterWasSet_[i])
                    continue;

                int32 queueIndex{0};
                auto queue = data.outputParameterChanges->addParameterData(parameterDefinitions_[i].id, queueIndex);

                int32 pointIndex{0};
                const auto* mapping = parameterMappings_.getParameter(parameterDefinitions_[i].id);
                const double normalized = mapping->toNormalized(parameterBuffers_[i][0]);
                queue->addPoint(data.numSamples - 1, normalized, pointIndex);
            }
        }
    }

    /** Here we go...the process call */
    Steinberg::tresult PLUGIN_API process (Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE
    {
        stateTransfer.accessTransferObject_rt([this] (const auto& stateChanges)
        {
            for(const auto& [id, value] : stateChanges)
            {
                if(id == bypassParameter_.getParamID())
                {
                    bypassParameter_.setValue(value);
                    continue;
                }

                for(auto& parameter : inputParameters_)
                {
                    if(parameter.getParamID() == id)
                    {
                        parameter.setValue(value);
                        break;
                    }
                }
            }
        });

        handleParameterChanges(data.inputParameterChanges);

        // Bypass is processed at block granularity.
        const bool bypassed = bypassParameter_.advance(data.numSamples) >= 0.5;
        bypassProcessor32_.setActive(bypassed);
        bypassProcessor64_.setActive(bypassed);

        for (auto& inputParameter : inputParameters_)
        {
            const auto id = inputParameter.getParamID();
            const auto definition = std::ranges::find(parameterDefinitions_, id, &Parameter::id);
            const auto parameterIndex = static_cast<std::size_t>(std::distance(parameterDefinitions_.begin(), definition));

            const auto* mapping = parameterMappings_.getParameter(id);

            for (int sample = 0; sample < data.numSamples; ++sample)
            {
                parameterBuffers_[parameterIndex][sample] = mapping->toPlain(inputParameter.advance(1));
            }
        }

        outputParameterWasSet_.fill(false);
        ParameterValues parameterValues {parameterDefinitions_, parameterBuffers_, outputParameterWasSet_};

        // process audio here
        if (data.numInputs > 0 || data.numOutputs > 0)
        {
            if(processSetup.symbolicSampleSize == Vst::kSample32)
                processAudio<Vst::kSample32>(data, parameterValues);
            else
                processAudio<Vst::kSample64>(data, parameterValues);
        }

        bypassParameter_.endChanges();
        for(auto& parameter : inputParameters_)
        {
            parameter.endChanges();
        }
        
        publishOutputParameterChanges(data);

        return kResultOk;
    }

    template <Vst::SymbolicSampleSizes SampleSize>
    void processAudio(Vst::ProcessData& data, const ParameterValues& parameterValues)
    {
        using SampleT = std::conditional_t<SampleSize == Vst::kSample32, float, double>;

        Vst::AudioBusBuffers* outputs = data.outputs;
        auto outputBuffers = Vst::getChannelBuffers<SampleSize>(*outputs);
        auto outputSpan = std::span<SampleT* const>(outputBuffers, outputs->numChannels);

        if constexpr(PluginType::isInstrument)
        {
            outputs->silenceFlags = 0;

            std::span<const MidiEvent> midiEvents{};

            plugin_.template process<SampleT>(outputSpan, data.numSamples, midiEvents, parameterValues);
        }
        else
        {
            Vst::AudioBusBuffers* inputs = data.inputs;
            auto inputBuffers = Vst::getChannelBuffers<SampleSize>(*inputs);
    
            if(inputs->silenceFlags == Vst::getChannelMask(inputs->numChannels))
            {
                outputs->silenceFlags = inputs->silenceFlags;
                for(int i = 0; i < inputs->numChannels; ++i)
                    if(inputBuffers[i] != outputBuffers[i])
                        std::memset(outputBuffers[i], 0, data.numSamples * sizeof(SampleT));
                return;
            }
    
            if(bypassProcessor32_.isActive())
            {
                if constexpr (SampleSize == Vst::kSample32)
                    bypassProcessor32_.process(data);
                else
                    bypassProcessor64_.process(data);
    
                outputs->silenceFlags = inputs->silenceFlags;
                return;
            }

            outputs->silenceFlags = 0;
            auto inputSpan = std::span<const SampleT* const>(inputBuffers, inputs->numChannels);
            plugin_.template process<SampleT>(inputSpan, outputSpan, data.numSamples, parameterValues);
        }
    }

    /** For persistence */
    Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) SMTG_OVERRIDE
    {
        IBStreamer streamer(state, kLittleEndian);

        uint32 numParams;
        if(!streamer.readInt32u(numParams))
            return kResultFalse;

        auto paramChanges = std::make_unique<ParameterVector>();
        Vst::ParamID paramID;
        Vst::ParamValue value;

        for(int i = 0; i < numParams; ++i)
        {
            if(!streamer.readInt32u(paramID))
                break;
            if(!streamer.readDouble(value))
                break;

            if(paramID == bypassParameter_.getParamID())
            {
                paramChanges->emplace_back(paramID, value);
                continue;
            }
            
            for(auto& parameter : inputParameters_)
            {
                if(parameter.getParamID() == paramID)
                {
                    paramChanges->emplace_back(paramID, value);
                    break;
                }
            }
        }

        stateTransfer.transferObject_ui(std::move(paramChanges));

        return kResultOk;
    }

    Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) SMTG_OVERRIDE
    {
        IBStreamer streamer(state, kLittleEndian);

        streamer.writeInt32u(1 + inputParameters_.size());
        streamer.writeInt32u(bypassParameter_.getParamID());
        streamer.writeDouble(bypassParameter_.getValue());

        for(auto& parameter : inputParameters_)
        {
            streamer.writeInt32u(parameter.getParamID());
            streamer.writeDouble(parameter.getValue());
        }

        return kResultOk;
    }

    Steinberg::tresult PLUGIN_API notify (Vst::IMessage* message) SMTG_OVERRIDE
    {
        if(!message)
            return kInvalidArgument;

        if(!std::strcmp(message->getMessageID(), "Singularity.Action"))
        {
            ActionValue value = std::monostate{};

            const void* nameData{nullptr};
            uint32 nameSize{0};

            int64 intValue{0};
            double floatValue{0.0};
            const void* stringData{nullptr};
            uint32 stringSize{0};

            auto attributes = message->getAttributes();

            attributes->getBinary("name", nameData, nameSize);


            if(attributes->getInt("boolValue", intValue) == kResultTrue)
                value = intValue != 0;
            else if(attributes->getInt("intValue", intValue) == kResultTrue)
                value = intValue;
            else if(attributes->getFloat("floatValue", floatValue) == kResultTrue)
                value = floatValue;
            else if(attributes->getBinary("stringValue", stringData, stringSize) == kResultTrue)
                value = std::string(static_cast<const char*>(stringData), stringSize);
            
            const auto result = plugin_.handleAction(static_cast<const char*>(nameData), value);

            // Response message back to controller
            if(result)
            {
                auto response = owned(allocateMessage());

                if(!response)
                    return kInvalidArgument;

                response->setMessageID("Singularity.ActionResult");
                response->getAttributes()->setBinary("name", result->first.data(), result->first.size());

                auto responseAttributes = response->getAttributes();

                std::visit(
                    [responseAttributes](const auto& payload)
                    {
                        using T = std::decay_t<decltype(payload)>;

                        if constexpr (std::is_same_v<T, bool>)
                            responseAttributes->setInt("boolValue", payload ? 1 : 0);
                        else if constexpr (std::is_same_v<T, std::int64_t>)
                            responseAttributes->setInt("intValue", payload);
                        else if constexpr (std::is_same_v<T, double>)
                            responseAttributes->setFloat("floatValue", payload);
                        else if constexpr (std::is_same_v<T, std::string>)
                            responseAttributes->setBinary("stringValue", payload.data(), payload.size()); 
                    }, 
                    result->second);

                    ComponentBase::sendMessage(response);
            }
            return kResultTrue;
        }

        return AudioEffect::notify (message);
    }

    void acquireNewExchangeBlock(uint16_t channelCount)
    {
        currentExchangeBlock = dataExchange->getCurrentOrNewBlock();
        if(auto block = toDataBlock(currentExchangeBlock))
        {
            block->sampleRate = static_cast<uint32_t>(processSetup.sampleRate);
            block->numChannels = channelCount;
            block->sampleSize = sizeof(float);
            block->numSamples = 0;
        }
    }

    template<typename SampleType>
    void sendUiData(AudioInputBuffer<SampleType> data, int numSamples)
    {
        if(currentExchangeBlock.blockID == Vst::InvalidDataExchangeBlockID)
        acquireNewExchangeBlock(data.size());
        
        auto totalSamples = numSamples;
        if(auto block = toDataBlock(currentExchangeBlock))
        {
            while (totalSamples > 0)
            {
                uint32_t numSamplesFreeInBlock = numSamplesToSend - block->numSamples;
                uint32_t numSamplesToCopy = std::min<uint32_t>(numSamplesFreeInBlock, totalSamples);
                for(auto channel = 0; channel < data.size(); ++channel)
                {
                    auto blockChannelData = block->samples + channel * static_cast<std::size_t>(numSamplesToSend)+ block->numSamples;
                    auto inputChannel = data[channel] + (numSamples - totalSamples);
                    std::copy_n(inputChannel, numSamplesToCopy, blockChannelData);
                }
                block->numSamples += numSamplesToCopy;
                if(block->numSamples == numSamplesToSend)
                {
                    dataExchange->sendCurrentBlock();
                    acquireNewExchangeBlock(data.size());
                    block = toDataBlock(currentExchangeBlock);
                    if(block == nullptr)
                        break;
                }
                totalSamples -= numSamplesToCopy;
            }
        }
    }




//------------------------------------------------------------------------
protected:
    PluginType plugin_;
    Vst::BypassProcessor<Vst::Sample32> bypassProcessor32_;
    Vst::BypassProcessor<Vst::Sample64> bypassProcessor64_;

    using ParameterDefinitions = std::remove_cvref_t<decltype(PluginType::getParameters())>;
    static constexpr std::size_t kPluginParameterCount = std::tuple_size_v<ParameterDefinitions>;
    ParameterDefinitions parameterDefinitions_ = PluginType::getParameters();
    std::array<std::vector<double>, kPluginParameterCount> parameterBuffers_;
    std::array<bool, kPluginParameterCount> outputParameterWasSet_{};

    // Vst::Sampl
    std::vector<Vst::SampleAccurate::Parameter> inputParameters_;
    Vst::SampleAccurate::Parameter bypassParameter_{Vst::kMaxParamId, 0.0};
    Vst::ParameterContainer parameterMappings_;

    using ParameterVector = std::vector<std::pair<Vst::ParamID, Vst::ParamValue>>;
    Vst::RTTransferT<ParameterVector> stateTransfer;
    std::unique_ptr<Vst::DataExchangeHandler> dataExchange;
    Vst::DataExchangeBlock currentExchangeBlock { InvalidDataExchangeBlock };
    uint16_t numChannels{0};
    uint32 numSamplesToSend{0};
};

//------------------------------------------------------------------------
