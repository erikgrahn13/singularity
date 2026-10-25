#pragma once

#include <stdint.h>
#include <array>

struct UiAudioBlock
{
    double sampleRate{};
    uint16_t numChannels{};
    uint32_t numSamples{};
    std::array<float, 1600> samples{};
};