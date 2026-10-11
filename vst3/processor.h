//------------------------------------------------------------------------
// Copyright(c) 2026 My Plug-in Company.
//------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/vstbypassprocessor.h"
#include "public.sdk/source/vst/vstaudioprocessoralgo.h"
#include "public.sdk/source/vst/utility/sampleaccurate.h"
#include "public.sdk/source/vst/utility/audiobuffers.h"
#include "public.sdk/source/vst/utility/rttransfer.h"

#include "plugincids.h"
#include "SingularityPlugin.h"

using namespace Steinberg;
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

        parameters_[0].setParamID(Steinberg::Vst::kMaxParamId);
        parameters_[0].setValue(0.0);

        const auto parameters = PluginType::getParameters();

        for(int i = 1; i < kParameterCount; ++i)
        {
            const auto& parameter = parameters[i - 1];
            const auto& definition = std::get<ContinuousParameter>(parameter.definition);

            parameters_[i].setParamID(parameter.id);
            parameters_[i].setValue(definition.defaultValue);
            parameterIds_[i - 1] = parameters_[i].getParamID();
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
            bypassProcessor32_.setup(*this, processSetup, getLatencySamples());
            bypassProcessor64_.setup(*this, processSetup, getLatencySamples());
        }
        else
        {
            bypassProcessor32_.reset();
            bypassProcessor64_.reset();
        }
        return AudioEffect::setActive(state);
    }

    /** Will be called before any process call */
    Steinberg::tresult PLUGIN_API setupProcessing (Steinberg::Vst::ProcessSetup& newSetup) SMTG_OVERRIDE
    {
        bypassProcessor32_.setup(*this, newSetup, getLatencySamples());
        bypassProcessor64_.setup(*this, newSetup, getLatencySamples());

        for(auto& parameterBuffer : parameterSampleBuffers_)
        {
            parameterBuffer.resize(static_cast<std::size_t>(newSetup.maxSamplesPerBlock));
        }
        return AudioEffect::setupProcessing (newSetup);
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
            for(auto& parameter : parameters_)
            {
                if(parameter.getParamID() == queue.getParameterId())
                {
                    parameter.beginChanges(&queue);
                    break;
                }
            }
        });
    }

    /** Here we go...the process call */
    Steinberg::tresult PLUGIN_API process (Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE
    {
        stateTransfer.accessTransferObject_rt([this] (const auto& stateChanges)
        {
            for(const auto& [id, value] : stateChanges)
            {
                for(auto& parameter : parameters_)
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
        const bool bypassed = parameters_[0].advance(data.numSamples) >= 0.5;

        bypassProcessor32_.setActive(bypassed);
        bypassProcessor64_.setActive(bypassed);

        // Generate sample-accurate buffers for plugin parameters only.
        for(int i = 1; i < kParameterCount; ++i)
        {
            for(int sample = 0; sample < data.numSamples; ++sample)
            {
                parameterSampleBuffers_[i - 1][sample] = parameters_[i].advance(1);
            }
        }

        ParameterValues parameterValues{ parameterIds_, parameterSampleBuffers_};

        // process audio here
        if (data.numInputs > 0 || data.numOutputs > 0)
        {
            if(processSetup.symbolicSampleSize == Vst::kSample32)
                processAudio<Vst::kSample32>(data, parameterValues);
            else
                processAudio<Vst::kSample64>(data, parameterValues);
        }

        for(auto& parameter : parameters_)
        {
            parameter.endChanges();
        }

        return kResultOk;
    }

    template <Vst::SymbolicSampleSizes SampleSize>
    void processAudio(Vst::ProcessData& data, const ParameterValues& parameterValues)
    {
        using SampleT = std::conditional_t<SampleSize == Vst::kSample32, float, double>;

        Vst::AudioBusBuffers* outputs = data.outputs;
        auto outputBuffers = Vst::getChannelBuffers<SampleSize>(*outputs);

        // Effect
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

        auto outputSpan = std::span<SampleT* const>(outputBuffers, outputs->numChannels);
        auto inputSpan = std::span<const SampleT* const>(inputBuffers, inputs->numChannels);

        plugin_.template process<SampleT>(inputSpan, outputSpan, data.numSamples, parameterValues);
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
            
            for(auto& parameter : parameters_)
            {
                if(parameter.getParamID() == paramID)
                {
                    paramChanges->emplace_back(std::make_pair(paramID, value));
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

        streamer.writeInt32u(parameters_.size());
        for(auto& parameter : parameters_)
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


//------------------------------------------------------------------------
protected:
    static constexpr std::size_t kParameterCount = 1 + std::tuple_size_v<decltype(PluginType::getParameters())>; // +1 is for the bypass
    PluginType plugin_;
    std::array<unsigned int, kParameterCount - 1> parameterIds_;
    std::array<std::vector<double>, kParameterCount - 1> parameterSampleBuffers_;
    Vst::BypassProcessor<Vst::Sample32> bypassProcessor32_;
    Vst::BypassProcessor<Vst::Sample64> bypassProcessor64_;
    std::array<Vst::SampleAccurate::Parameter, kParameterCount> parameters_; 

    using ParameterVector = std::vector<std::pair<Vst::ParamID, Vst::ParamValue>>;
    Vst::RTTransferT<ParameterVector> stateTransfer;
};

//------------------------------------------------------------------------
