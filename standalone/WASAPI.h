#pragma once

#include "ISingularityAudio.h"
#include <future>
#include <thread>

template<typename PluginType>
class WASAPI : public ISingularityAudio<PluginType>
{
    public:
    WASAPI();
    ~WASAPI();

    std::vector<AudioDevice> probeDevices() const override;

    private:
    void audioThreadMain(std::promise<void> startupPromise) noexcept;

    void* stopEvent_{nullptr};
    std::thread audioThread_;
};
