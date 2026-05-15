#pragma once
#include <vector>
#include <map>
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
std::map<int, double> getDefaultParams();

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
        PluginType::registerParameters();
        // Pre-populate _params with all registered defaults so the audio
        // thread only ever updates existing keys — never allocates
        for (auto& [id, value] : getDefaultParams())
            _params[id] = value;
    }

    // Called at the top of each audio callback: drains queue into local RT-safe array
    void processParameterChanges()
    {
        ParameterChange change;
        while (_paramChanges.pop(change))
            _params[change.id] = change.value;
    }

    std::map<int, double> _params;
    PluginType mPlugin;

    private:
    SingularityQueue<ParameterChange, 256> _paramChanges;
};
