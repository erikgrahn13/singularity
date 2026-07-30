#pragma once

#include "IParameterProvider.h"
#include "Vst3ProgramData.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstunits.h"
#include <algorithm>
#include <cmath>
#include <span>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Steinberg::SingularityVst3 {

inline constexpr int32 kComponentStateMagic = 0x53475354; // "SGST"
inline constexpr int32 kComponentStateVersion = 1;
inline constexpr int32 kMaximumStateProgramEntries = 1024;
inline constexpr int32 kMaximumStateProgramParameters = 65536;
inline constexpr int32 kMaximumStateProgramPayloadBytes =
    256 * 1024 * 1024;

struct ProgramSelection
{
    Vst::ProgramListID listId = Vst::kNoProgramListId;
    int32 programIndex = 0;
};

struct ProgramSlotState
{
    Vst::ProgramListID listId = Vst::kNoProgramListId;
    int32 programIndex = 0;
    ProgramData data;
};

struct ComponentState
{
    bool bypass = false;
    std::vector<SerializedParameter> parameterValues;
    std::vector<ProgramSelection> programSelections;
    std::vector<ProgramSlotState> modifiedPrograms;
};

inline bool isWritableParameter(
    std::span<const ::Parameter> parameters,
    Vst::ParamID id)
{
    return std::ranges::any_of(
        parameters,
        [id](const auto& parameter)
        {
            return parameter.id == id && !parameter.readOnly;
        });
}

inline bool writeProgramState(
    IBStream* stream,
    IBStreamer& streamer,
    const ComponentState& state)
{
    std::size_t serializedParameters = 0;
    std::size_t serializedPayloadBytes = 0;
    if (state.programSelections.size() >
            static_cast<std::size_t>(kMaximumStateProgramEntries) ||
        state.modifiedPrograms.size() >
            static_cast<std::size_t>(kMaximumStateProgramEntries) ||
        !streamer.writeInt32(
            static_cast<int32>(state.programSelections.size())))
        return false;

    for (const auto& selection : state.programSelections)
    {
        if (selection.listId < 0 || selection.programIndex < 0 ||
            !streamer.writeInt32(selection.listId) ||
            !streamer.writeInt32(selection.programIndex))
            return false;
    }

    if (!streamer.writeInt32(
            static_cast<int32>(state.modifiedPrograms.size())))
        return false;
    for (const auto& program : state.modifiedPrograms)
    {
        serializedParameters += program.data.parameters.size();
        serializedPayloadBytes += program.data.payload.size();
        if (program.listId < 0 || program.programIndex < 0 ||
            serializedParameters >
                static_cast<std::size_t>(
                    kMaximumStateProgramParameters) ||
            serializedPayloadBytes >
                static_cast<std::size_t>(
                    kMaximumStateProgramPayloadBytes) ||
            !streamer.writeInt32(program.listId) ||
            !streamer.writeInt32(program.programIndex) ||
            !writeProgramData(stream, program.data))
            return false;
    }
    return true;
}

inline bool readProgramState(
    IBStream* stream,
    IBStreamer& streamer,
    ComponentState& state,
    bool includesModifiedPrograms)
{
    int32 selectionCount = 0;
    if (!streamer.readInt32(selectionCount) || selectionCount < 0 ||
        selectionCount > kMaximumStateProgramEntries)
        return false;
    state.programSelections.reserve(
        static_cast<std::size_t>(selectionCount));
    for (int32 index = 0; index < selectionCount; ++index)
    {
        ProgramSelection selection;
        if (!streamer.readInt32(selection.listId) ||
            !streamer.readInt32(selection.programIndex) ||
            selection.listId < 0 || selection.programIndex < 0)
            return false;
        state.programSelections.push_back(selection);
    }

    if (!includesModifiedPrograms)
        return true;

    int32 modifiedCount = 0;
    if (!streamer.readInt32(modifiedCount) || modifiedCount < 0 ||
        modifiedCount > kMaximumStateProgramEntries)
        return false;
    state.modifiedPrograms.reserve(static_cast<std::size_t>(modifiedCount));
    int32 remainingParameters = kMaximumStateProgramParameters;
    int32 remainingPayloadBytes = kMaximumStateProgramPayloadBytes;
    for (int32 index = 0; index < modifiedCount; ++index)
    {
        ProgramSlotState program;
        if (!streamer.readInt32(program.listId) ||
            !streamer.readInt32(program.programIndex) ||
            program.listId < 0 || program.programIndex < 0 ||
            !readProgramData(
                stream,
                program.data,
                remainingParameters,
                std::min(
                    remainingPayloadBytes,
                    kMaximumSerializedPayloadBytes)))
            return false;
        remainingParameters -=
            static_cast<int32>(program.data.parameters.size());
        remainingPayloadBytes -=
            static_cast<int32>(program.data.payload.size());
        state.modifiedPrograms.push_back(std::move(program));
    }
    return true;
}

inline bool writeComponentState(
    IBStream* stream,
    std::span<const ::Parameter> parameters,
    const ComponentState& state)
{
    if (!stream)
        return false;

    IBStreamer streamer(stream, kLittleEndian);
    if (!streamer.writeInt32(kComponentStateMagic) ||
        !streamer.writeInt32(kComponentStateVersion) ||
        !streamer.writeInt32(state.bypass ? 1 : 0))
        return false;

    const auto writableCount = std::ranges::count_if(
        parameters,
        [](const auto& parameter) { return !parameter.readOnly; });
    if (writableCount > kMaximumSerializedParameters ||
        !streamer.writeInt32(static_cast<int32>(writableCount)))
        return false;

    std::unordered_set<Vst::ParamID> writtenIds;
    for (const auto& parameter : parameters)
    {
        if (parameter.readOnly)
            continue;

        auto value = 0.0;
        auto found = false;
        for (const auto& [id, candidate] : state.parameterValues)
        {
            if (id == parameter.id)
            {
                value = candidate;
                found = true;
                break;
            }
        }
        if (!found || !std::isfinite(value) ||
            !writtenIds.insert(parameter.id).second ||
            !streamer.writeInt32(static_cast<int32>(parameter.id)) ||
            !streamer.writeDouble(value))
            return false;
    }

    return writeProgramState(stream, streamer, state);
}

inline bool readVersionedComponentState(
    IBStream* stream,
    std::span<const ::Parameter> parameters,
    IBStreamer& streamer,
    ComponentState& state)
{
    int32 version = 0;
    int32 bypass = 0;
    int32 parameterCount = 0;
    if (!streamer.readInt32(version) ||
        version != kComponentStateVersion ||
        !streamer.readInt32(bypass) ||
        (bypass != 0 && bypass != 1) ||
        !streamer.readInt32(parameterCount) ||
        parameterCount < 0 ||
        parameterCount > kMaximumSerializedParameters)
        return false;

    ComponentState decoded;
    decoded.bypass = bypass != 0;
    std::unordered_set<Vst::ParamID> decodedIds;
    decodedIds.reserve(static_cast<std::size_t>(parameterCount));
    for (int32 index = 0; index < parameterCount; ++index)
    {
        int32 rawId = 0;
        double value = 0.0;
        if (!streamer.readInt32(rawId) || rawId < 0 ||
            !streamer.readDouble(value) || !std::isfinite(value))
            return false;

        const auto id = static_cast<Vst::ParamID>(rawId);
        if (id > Vst::kMaxParamId || !decodedIds.insert(id).second)
            return false;
        if (isWritableParameter(parameters, id))
            decoded.parameterValues.emplace_back(
                id, std::clamp(value, 0.0, 1.0));
    }

    if (!readProgramState(stream, streamer, decoded, true))
        return false;
    state = std::move(decoded);
    return true;
}

inline bool readLegacyComponentState(
    IBStream* stream,
    std::span<const ::Parameter> parameters,
    IBStreamer& streamer,
    int32 bypass,
    ComponentState& state)
{
    ComponentState decoded;
    decoded.bypass = bypass != 0;
    for (const auto& parameter : parameters)
    {
        if (parameter.readOnly)
            continue;

        double value = 0.0;
        if (!streamer.readDouble(value))
            break;
        if (!std::isfinite(value))
            return false;
        decoded.parameterValues.emplace_back(
            parameter.id, std::clamp(value, 0.0, 1.0));
    }

    int32 magic = 0;
    if (streamer.readInt32(magic) && magic == kStateExtensionMagic)
    {
        int32 version = 0;
        if (!streamer.readInt32(version))
            return false;
        if (version == 1)
        {
            int32 programIndex = 0;
            if (!streamer.readInt32(programIndex) || programIndex < 0)
                return false;
            decoded.programSelections.push_back(
                {Vst::kNoProgramListId, programIndex});
        }
        else if (version == 2 || version == kStateExtensionVersion)
        {
            if (!readProgramState(
                    stream, streamer, decoded, version >= 3))
                return false;
        }
        else
        {
            return false;
        }
    }

    state = std::move(decoded);
    return true;
}

inline bool readComponentState(
    IBStream* stream,
    std::span<const ::Parameter> parameters,
    ComponentState& state)
{
    if (!stream)
        return false;

    IBStreamer streamer(stream, kLittleEndian);
    int32 magicOrLegacyBypass = 0;
    if (!streamer.readInt32(magicOrLegacyBypass))
        return false;

    if (magicOrLegacyBypass == kComponentStateMagic)
        return readVersionedComponentState(
            stream, parameters, streamer, state);
    return readLegacyComponentState(
        stream, parameters, streamer, magicOrLegacyBypass, state);
}

} // namespace Steinberg::SingularityVst3
