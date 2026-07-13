#pragma once
#include <concepts>
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <limits>
#include <functional>
#include "../utilities/SingularityQueue.h"
#include "../IParameterProvider.h"
#include "SingularityPlugin.h"
#include "../AudioDataExchange.h"

IParameterProvider& getParameterContainer();
void setOnParameterChanged(std::function<void(int, double)> cb);
void populateParameterContainer(std::span<const Parameter> params);
void setOutputParameter(int id, double value);

class AudioDevice {
    public:
    int id;
    std::string name; 
    int inputChannels;
    int outputChannels;
    int minBufferSize;
    int maxBufferSize;
    int preferredBufferSize;
    double sampleRate;

    std::vector<int> supportedSampleRates;
};

template<::SingularityPlugin PluginType>
class ISingularityAudio
{
    public:
    virtual ~ISingularityAudio() = default;
    virtual std::vector<AudioDevice> probeDevices() const = 0;

    // Called from GUI thread: queues a parameter change for the audio thread
    void pushParameterChange(int id, double value)
    {
        _paramChanges.push({id, value});
    }

    Singularity::AudioDataExchange::AudioDataQueue& audioDataQueue() { return _audioDataQueue; }

    protected:
    ISingularityAudio()
    {
        auto params = PluginType::getParameters ();
        populateParameterContainer (params);
        for (auto& p : params)
        {
            _params.push_back ({p.id, p.defaultValue});
            if (p.readOnly)
                _outputParameters.push_back({p.id, p.defaultValue});
        }
    }

    // Called at the top of each audio callback: drains queue into local RT-safe array
    void processParameterChanges()
    {
        ParameterChange change;
        while (_paramChanges.pop(change))
        {
            bool isOutput = false;
            for (auto& output : _outputParameters)
                if (output.first == static_cast<unsigned int>(change.id))
                {
                    isOutput = true;
                    break;
                }
            if (isOutput) continue;

            for (auto& [id, val] : _params)
                if (id == (unsigned int)change.id) { val = change.value; break; }
        }
    }

    // Call once when sample rate and block size are first known.
    // Safe to call from a real-time thread on first process callback.
    void callPrepare(double sampleRate, int maxBlockSize)
    {
        if (_prepared) return;
        _sampleRate = sampleRate;
        _prepared = true;
        mPlugin.prepare(sampleRate, maxBlockSize);
    }

    void publishOutputParameters()
    {
        for (auto& output : _outputParameters)
            for (auto& [id, value] : _params)
                if (id == output.first)
                {
                    setOutputParameter(static_cast<int>(id), value);
                    break;
                }
    }

    void resetOutputParameters()
    {
        for (auto& [outputId, defaultValue] : _outputParameters)
            for (auto& [id, value] : _params)
                if (id == outputId)
                {
                    value = defaultValue;
                    break;
                }
    }

    template<typename SampleT>
    void processInstrument(std::span<SampleT* const> outputs, int numSamples, std::span<const MidiEvent> midiEvents)
    {
        Singularity::AudioDataExchange::ScopedSendContext sendContext(&_audioDataQueue, _sampleRate);
        mPlugin.template process<SampleT>(outputs, numSamples, midiEvents, ParamList{_params});
    }

    template<typename SampleT>
    void processEffect(std::span<const SampleT* const> inputs, std::span<SampleT* const> outputs, int numSamples)
    {
        Singularity::AudioDataExchange::ScopedSendContext sendContext(&_audioDataQueue, _sampleRate);
        mPlugin.template process<SampleT>(inputs, outputs, numSamples, ParamList{_params});
    }

    std::vector<std::pair<unsigned int, double>> _params;
    PluginType mPlugin;

    private:
    bool _prepared = false;
    double _sampleRate = 0.0;
    Singularity::AudioDataExchange::AudioDataQueue _audioDataQueue;
    SingularityQueue<ParameterChange, 256> _paramChanges;
    std::vector<std::pair<unsigned int, double>> _outputParameters;
};
