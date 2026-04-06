#pragma once
#include <vector>
#include <memory>

class AudioDevice {
    public:
    int id;
    std::string name; 
    int inputChannels;
    int outputChannels;
    std::vector<int> supportedSampleRates;
};

class ISingularityAudio
{
    public:
    static std::unique_ptr<ISingularityAudio> createSingularityAudio();
    virtual ~ISingularityAudio() = default;
    virtual std::vector<AudioDevice> probeDevices() const = 0;
};
