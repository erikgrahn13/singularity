#pragma once

#include "IParameterProvider.h"
#include "pluginterfaces/vst/vsttypes.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include <algorithm>
#include <cmath>
#include <string>

namespace Steinberg::SingularityVst3 {

constexpr int32 kStateMagic = 0x53475031; // "SGP1"
constexpr int32 kStateVersion = 1;

inline void copyAsciiToString128(const std::string& source, Vst::String128 target)
{
    for (int i = 0; i < 127 && i < static_cast<int>(source.size()); ++i)
        target[i] = source[static_cast<std::size_t>(i)];
}

inline int32 stepCountFor(const ::Parameter& parameter)
{
    switch (parameter.type)
    {
        case ParamType::Bool:
            return 1;
        case ParamType::Choice:
            return static_cast<int32>(parameter.choices.empty() ? 0 : parameter.choices.size() - 1);
        case ParamType::Stepped:
            return std::max<int32>(1, parameter.steps - 1);
        case ParamType::Float:
        default:
            return 0;
    }
}

inline int32 flagsFor(const ::Parameter& parameter)
{
    int32 flags = 0;
    if (parameter.automatable)
        flags |= Vst::ParameterInfo::kCanAutomate;
    if (parameter.readOnly)
        flags |= Vst::ParameterInfo::kIsReadOnly;
    if (parameter.wrapAround)
        flags |= Vst::ParameterInfo::kIsWrapAround;
    if (parameter.isBypass)
        flags |= Vst::ParameterInfo::kIsBypass;
    if (parameter.isList || parameter.type == ParamType::Choice || !parameter.choices.empty())
        flags |= Vst::ParameterInfo::kIsList;
    if (parameter.isProgramChange)
        flags |= Vst::ParameterInfo::kIsProgramChange;
    return flags;
}

inline double plainToNormalized(const ::Parameter& parameter, double plainValue)
{
    if (parameter.type == ParamType::Bool)
        return plainValue >= 0.5 ? 1.0 : 0.0;

    if (parameter.type == ParamType::Choice && !parameter.choices.empty())
    {
        const auto maxIndex = static_cast<double>(parameter.choices.size() - 1);
        if (maxIndex <= 0.0)
            return 0.0;
        return std::clamp(std::round(plainValue) / maxIndex, 0.0, 1.0);
    }

    if (parameter.maxValue == parameter.minValue)
        return 0.0;

    return std::clamp((plainValue - parameter.minValue) /
        (parameter.maxValue - parameter.minValue), 0.0, 1.0);
}

inline double normalizedToPlain(const ::Parameter& parameter, double normalizedValue)
{
    const auto clamped = std::clamp(normalizedValue, 0.0, 1.0);

    if (parameter.type == ParamType::Bool)
        return clamped >= 0.5 ? 1.0 : 0.0;

    if (parameter.type == ParamType::Choice && !parameter.choices.empty())
        return std::round(clamped * static_cast<double>(parameter.choices.size() - 1));

    const auto plain = parameter.minValue + clamped * (parameter.maxValue - parameter.minValue);
    if (parameter.type == ParamType::Stepped)
        return std::round(plain);

    return plain;
}

} // namespace Steinberg::SingularityVst3
