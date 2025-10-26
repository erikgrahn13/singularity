#pragma once

#include "../../ISingularityAudioBackend.h"
#include "asio.h"
#include "asiodrivers.h"

class SingularityASIO : public ISingularityAudioBackend
{
  public:
    SingularityASIO() : ISingularityAudioBackend("ASIO")
    {
    }
    std::vector<AudioDeviceInfo> probeAudioDevices() override;
};