#pragma once
#include <vector>
#include <memory>
#include <string>
#include "../utilities/SingularityQueue.h"
#include "../IParameterProvider.h"

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

class ISingularityAudio
{
    public:
    static std::vector<std::string> backends;

    static std::unique_ptr<ISingularityAudio> createSingularityAudio();

    virtual ~ISingularityAudio() = default;
    virtual std::vector<AudioDevice> probeDevices() const = 0;

    // Called from GUI thread: queues a parameter change for the audio thread
    void pushParameterChange(int id, double value)
    {
        _paramChanges.push({id, value});
    }

    protected:
    ISingularityAudio(std::string type)
    {
        backends.push_back(type);
    }

    // Called at the top of each audio callback: drains queue into local RT-safe array
    void processParameterChanges()
    {
        ParameterChange change;
        while (_paramChanges.pop(change))
            _params[change.id] = change.value;
    }

    double _params[256]{}; // RT-safe local param copy, only written by audio thread

    private:
    SingularityQueue<ParameterChange, 256> _paramChanges;
};
