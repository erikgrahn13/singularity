#include "SingularityASIO.h"
#include "Windows.h"
#include <iostream>

extern AsioDrivers *asioDrivers;
bool loadAsioDriver(char *name);

std::vector<AudioDeviceInfo> SingularityASIO::probeAudioDevices()
{
    std::vector<std::string> names(16, std::string(32, '\0'));
    std::vector<char *> namePtrs;
    int numDrivers;

    for (auto &name : names)
    {
        namePtrs.push_back(&name[0]);
    }

    {
        // Needs an ASIO driver instance to be able to query the available drivers
        AsioDrivers tmpDriver;
        numDrivers = tmpDriver.getDriverNames(namePtrs.data(), 16);
    }

    for (int i = 0; i < numDrivers; ++i)
    {
        AudioDeviceInfo info = {
            .name = names[i],
            .id = names[i],
        };

        drivers.push_back(info);
    }

    return drivers;
}