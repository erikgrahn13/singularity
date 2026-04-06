#pragma once
#include <vector>
#include <memory>
#include <string>

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

    protected:
    ISingularityAudio(std::string type)
    {
        backends.push_back(type);
    }
    // std::string deviceType;
};
