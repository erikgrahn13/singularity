#include "PipeWire.h"

PipeWire::PipeWire() : ISingularityAudio("PipeWire")
{

}

PipeWire::~PipeWire()
{

}

std::vector<AudioDevice> PipeWire::probeDevices() const
{
    return {};
}