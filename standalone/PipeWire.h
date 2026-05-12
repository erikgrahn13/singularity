#pragma once

#include "ISingularityAudio.h"

class PipeWire : public ISingularityAudio 
{
    public:
    PipeWire();

    ~PipeWire();

    std::vector<AudioDevice> probeDevices() const override;
};