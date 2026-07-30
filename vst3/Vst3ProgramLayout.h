#pragma once

#include "BuiltInProgram.h"
#include <algorithm>
#include <cstdint>
#include <span>
#include <string>
#include <unordered_set>
#include <vector>

struct Vst3ProgramUnit
{
    // Unit zero is implicit and reserved for the root.
    int32_t id = 0;
    int32_t parentId = 0;
    std::string name;

    // Optional zero-based event-bus and MIDI-channel association. Set either
    // value to -1 when the unit is not tied to a MIDI input channel.
    int32_t eventBusIndex = -1;
    int32_t midiChannel = -1;
};

struct Vst3ProgramListBinding
{
    std::string collectionId;
    int32_t unitId = 0;
};

inline bool validateVst3ProgramUnits(std::span<const Vst3ProgramUnit> units)
{
    std::unordered_set<int32_t> ids {0};
    std::unordered_set<uint64_t> midiMappings;
    for (const auto& unit : units)
    {
        if (unit.id <= 0 || unit.name.empty() ||
            unit.eventBusIndex < -1 || unit.midiChannel < -1 ||
            unit.midiChannel > 15 ||
            ((unit.eventBusIndex < 0) != (unit.midiChannel < 0)) ||
            !ids.insert(unit.id).second)
            return false;

        if (unit.midiChannel >= 0)
        {
            const auto mapping =
                (static_cast<uint64_t>(
                     static_cast<uint32_t>(unit.eventBusIndex)) << 32u) |
                static_cast<uint32_t>(unit.midiChannel);
            if (!midiMappings.insert(mapping).second)
                return false;
        }
    }

    for (const auto& unit : units)
    {
        if (!ids.contains(unit.parentId))
            return false;

        std::unordered_set<int32_t> ancestors;
        auto parentId = unit.parentId;
        while (parentId != 0)
        {
            if (!ancestors.insert(parentId).second)
                return false;

            const auto parent = std::find_if(
                units.begin(),
                units.end(),
                [parentId](const auto& candidate)
                {
                    return candidate.id == parentId;
                });
            if (parent == units.end())
                return false;
            parentId = parent->parentId;
        }
    }
    return true;
}

template<typename P>
concept HasVst3ProgramUnits = requires
{
    { P::getVst3ProgramUnits() };
};

template<typename P>
concept HasVst3ProgramListBindings = requires
{
    { P::getVst3ProgramListBindings() };
};

template<typename P>
concept HasVst3ProgramLists =
    HasProgramCollections<P> && HasVst3ProgramListBindings<P>;
