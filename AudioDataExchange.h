#pragma once

#include "utilities/SingularityQueue.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>

namespace Singularity::AudioDataExchange {

constexpr std::uint32_t kDefaultContextID = 1;
constexpr std::uint32_t kMaxFloatSamples = 2048;

struct AudioDataBlock
{
    std::uint32_t contextID = kDefaultContextID;
    std::uint32_t sampleRate = 0;
    std::uint16_t sampleSize = sizeof(float);
    std::uint16_t numChannels = 1;
    std::uint32_t numSamples = 0;
    float samples[kMaxFloatSamples]{};
};

class IDataSink
{
public:
    virtual ~IDataSink() = default;
    virtual void pushAudioDataBlock(const AudioDataBlock& block) = 0;
};

class AudioDataQueue final : public IDataSink
{
public:
    void pushAudioDataBlock(const AudioDataBlock& block) override { queue_.push(block); }
    bool popAudioDataBlock(AudioDataBlock& block) { return queue_.pop(block); }

private:
    SingularityQueue<AudioDataBlock, 64> queue_;
};

template<typename SampleT>
inline void appendInterleavedFloatData(AudioDataBlock& block, double sampleRate, const SampleT* const* channels,
                                       int numChannels, int numSamples)
{
    if (!channels || numChannels <= 0 || numSamples <= 0)
        return;

    block.sampleRate = static_cast<std::uint32_t>(sampleRate);
    block.sampleSize = sizeof(float);
    block.numChannels = static_cast<std::uint16_t>(std::min(numChannels, 32));

    const auto framesAvailable = (kMaxFloatSamples - block.numSamples) / block.numChannels;
    const auto framesToWrite = std::min<std::uint32_t>(framesAvailable, static_cast<std::uint32_t>(numSamples));
    for (std::uint32_t frame = 0; frame < framesToWrite; ++frame)
        for (std::uint16_t channel = 0; channel < block.numChannels; ++channel)
            block.samples[block.numSamples++] = channels[channel] ? static_cast<float>(channels[channel][frame]) : 0.0f;
}

struct SendContext
{
    IDataSink* sink = nullptr;
    double sampleRate = 0.0;
};

#if !defined(SINGULARITY_CAPI)
inline thread_local SendContext currentSendContext;

class ScopedSendContext
{
public:
    ScopedSendContext(IDataSink* sink, double sampleRate)
        : previous_(currentSendContext)
    {
        currentSendContext = {sink, sampleRate};
    }

    ~ScopedSendContext()
    {
        currentSendContext = previous_;
    }

    ScopedSendContext(const ScopedSendContext&) = delete;
    ScopedSendContext& operator=(const ScopedSendContext&) = delete;

private:
    SendContext previous_;
};
#else
class ScopedSendContext
{
public:
    ScopedSendContext(IDataSink*, double) {}
};
#endif

template<typename SampleT>
inline void sendAudioDataToUI(std::span<const SampleT* const> channels, int numSamples)
{
#if defined(SINGULARITY_CAPI)
    static_cast<void>(channels);
    static_cast<void>(numSamples);
#else
    if (!currentSendContext.sink || channels.empty() || numSamples <= 0)
        return;

    std::array<const SampleT*, 32> offsetChannels {};
    const auto numChannels = std::min(channels.size(), offsetChannels.size());
    int frameOffset = 0;
    while (frameOffset < numSamples)
    {
        for (std::size_t channel = 0; channel < numChannels; ++channel)
            offsetChannels[channel] = channels[channel]
                ? channels[channel] + frameOffset
                : nullptr;

        AudioDataBlock block;
        appendInterleavedFloatData(
            block,
            currentSendContext.sampleRate,
            offsetChannels.data(),
            static_cast<int>(numChannels),
            numSamples - frameOffset);
        if (block.numChannels == 0 || block.numSamples == 0)
            break;

        currentSendContext.sink->pushAudioDataBlock(block);
        frameOffset += static_cast<int>(block.numSamples / block.numChannels);
    }
#endif
}

template<typename SampleT>
inline void sendAudioDataToUI(std::span<SampleT* const> channels, int numSamples)
{
    sendAudioDataToUI(std::span<const SampleT* const>(channels.data(), channels.size()), numSamples);
}

inline AudioDataBlock* toAudioDataBlock(void* data) { return reinterpret_cast<AudioDataBlock*>(data); }

} // namespace Singularity::AudioDataExchange
