#pragma once

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/vsttypes.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Steinberg::SingularityVst3 {

inline constexpr int32 kProgramDataMagic = 0x53505247; // "SPRG"
inline constexpr int32 kProgramDataVersion = 2;
inline constexpr int32 kStateExtensionMagic = 0x53505354; // "SPST"
inline constexpr int32 kStateExtensionVersion = 3;
inline constexpr int32 kMaximumSerializedParameters = 65536;
inline constexpr int32 kMaximumSerializedPayloadBytes = 64 * 1024 * 1024;

using SerializedParameter = std::pair<Vst::ParamID, Vst::ParamValue>;

struct ProgramData
{
    std::vector<SerializedParameter> parameters;
    std::vector<std::byte> payload;
};

// Program-list IDs and their selector parameter IDs share a stable value.
// Keeping them in the upper half of the positive ParamID range avoids the
// small IDs normally chosen by plug-in authors; collisions are still checked
// during initialization.
inline Vst::ParamID programListId(std::string_view stableBankId)
{
    uint32_t hash = 2166136261u;
    for (const auto character : stableBankId)
    {
        hash ^= static_cast<uint8_t>(character);
        hash *= 16777619u;
    }
    return static_cast<Vst::ParamID>(
        0x40000000u | (hash & 0x3ffffffeu));
}

inline bool writeProgramData(
    IBStream* stream,
    const ProgramData& data)
{
    if (!stream ||
        data.parameters.size() >
            static_cast<std::size_t>(kMaximumSerializedParameters) ||
        data.payload.size() >
            static_cast<std::size_t>(kMaximumSerializedPayloadBytes))
        return false;

    IBStreamer streamer(stream, kLittleEndian);
    if (!streamer.writeInt32(kProgramDataMagic) ||
        !streamer.writeInt32(kProgramDataVersion) ||
        !streamer.writeInt32(static_cast<int32>(data.parameters.size())))
        return false;

    for (const auto& [id, value] : data.parameters)
    {
        if (id > Vst::kMaxParamId || !std::isfinite(value) ||
            !streamer.writeInt32(static_cast<int32>(id)) ||
            !streamer.writeDouble(value))
            return false;
    }

    if (!streamer.writeInt32(static_cast<int32>(data.payload.size())))
        return false;
    if (data.payload.empty())
        return true;

    int32 bytesWritten = 0;
    return stream->write(
               const_cast<std::byte*>(data.payload.data()),
               static_cast<int32>(data.payload.size()),
               &bytesWritten) == kResultTrue &&
        bytesWritten == static_cast<int32>(data.payload.size());
}

inline bool readProgramData(
    IBStream* stream,
    ProgramData& data,
    int32 maximumParameters = kMaximumSerializedParameters,
    int32 maximumPayloadBytes = kMaximumSerializedPayloadBytes)
{
    if (!stream || maximumParameters < 0 ||
        maximumParameters > kMaximumSerializedParameters ||
        maximumPayloadBytes < 0 ||
        maximumPayloadBytes > kMaximumSerializedPayloadBytes)
        return false;

    IBStreamer streamer(stream, kLittleEndian);
    int32 magic = 0;
    int32 version = 0;
    int32 count = 0;
    if (!streamer.readInt32(magic) || magic != kProgramDataMagic ||
        !streamer.readInt32(version) ||
        (version != 1 && version != kProgramDataVersion) ||
        !streamer.readInt32(count) || count < 0 ||
        count > maximumParameters)
        return false;

    std::vector<SerializedParameter> decoded;
    decoded.reserve(static_cast<std::size_t>(count));
    std::unordered_set<Vst::ParamID> decodedIds;
    decodedIds.reserve(static_cast<std::size_t>(count));
    for (int32 index = 0; index < count; ++index)
    {
        int32 rawId = 0;
        double value = 0.0;
        if (!streamer.readInt32(rawId) || rawId < 0 ||
            !streamer.readDouble(value) || !std::isfinite(value))
            return false;

        const auto id = static_cast<Vst::ParamID>(rawId);
        if (id > Vst::kMaxParamId)
            return false;
        if (!decodedIds.insert(id).second)
            return false;

        decoded.emplace_back(id, std::clamp(value, 0.0, 1.0));
    }

    std::vector<std::byte> payload;
    if (version >= 2)
    {
        int32 payloadSize = 0;
        if (!streamer.readInt32(payloadSize) || payloadSize < 0 ||
            payloadSize > maximumPayloadBytes)
            return false;

        payload.resize(static_cast<std::size_t>(payloadSize));
        if (payloadSize > 0)
        {
            int32 bytesRead = 0;
            if (stream->read(payload.data(), payloadSize, &bytesRead) !=
                    kResultTrue ||
                bytesRead != payloadSize)
                return false;
        }
    }

    data.parameters = std::move(decoded);
    data.payload = std::move(payload);
    return true;
}

} // namespace Steinberg::SingularityVst3
