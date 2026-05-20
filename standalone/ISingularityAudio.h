#pragma once
#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <limits>
#include <functional>
#include "../utilities/SingularityQueue.h"
#include "../IParameterProvider.h"
#include "../SingularityPlugin.h"

IParameterProvider& getParameterContainer();
void setOnParameterChanged(std::function<void(int, double)> cb);
void populateParameterContainer(std::span<const Parameter> params);

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

    protected:
    ISingularityAudio()
    {
        auto params = PluginType::getParameters ();
        populateParameterContainer (params);
        for (auto& p : params)
            _params.push_back ({p.id, p.defaultValue});
    }

    // Called at the top of each audio callback: drains queue into local RT-safe array
    void processParameterChanges()
    {
        ParameterChange change;
        while (_paramChanges.pop(change))
            for (auto& [id, val] : _params)
                if (id == (unsigned int)change.id) { val = change.value; break; }
    }

    // Call once when sample rate and block size are first known.
    // Safe to call from a real-time thread on first process callback.
    void callPrepare(double sampleRate, int maxBlockSize)
    {
        if (_prepared) return;
        _prepared = true;
        mPlugin.prepare(sampleRate, maxBlockSize);
    }

    std::vector<std::pair<unsigned int, double>> _params;
    PluginType mPlugin;

    private:
    bool _prepared = false;
    SingularityQueue<ParameterChange, 256> _paramChanges;
};
