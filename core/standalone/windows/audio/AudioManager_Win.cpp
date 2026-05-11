#include "AudioManager.h"
#include "SingularityASIO.h"

AudioManager::AudioManager()
{
    backends.push_back(std::make_unique<SingularityASIO>());

    probeDevices();
}

void AudioManager::probeDevices()
{
    for (auto &backend : backends)
    {
        auto devices = backend->probeAudioDevices();

        allDevices.insert(allDevices.end(), devices.begin(), devices.end());
    }
}

const std::vector<AudioDeviceInfo> &AudioManager::getDeviceList() const
{
    return allDevices;
}
