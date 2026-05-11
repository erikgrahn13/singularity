#pragma once

#include <string>
#include <vector>

struct AudioDeviceInfo
{
    std::string name;
    std::string id;
    // std::string backendType;
    int numInputChannels;
    int numOutputChannels;
    int sampleRate;
};

class ISingularityAudioBackend
{
  public:
    ISingularityAudioBackend(const std::string &name) : typeName(name)
    {
    }
    virtual std::vector<AudioDeviceInfo> probeAudioDevices() = 0;

  protected:
    std::string typeName;
    std::vector<AudioDeviceInfo> drivers;
};