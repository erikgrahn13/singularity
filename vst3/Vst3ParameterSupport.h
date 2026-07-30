#pragma once

#include "IParameterProvider.h"
#include <algorithm>
#include <cmath>

namespace Steinberg::SingularityVst3 {

inline double plainToNormalized(
    const ::Parameter& parameter,
    double plainValue)
{
    if (parameter.type == ParamType::Bool)
        return plainValue >= 0.5 ? 1.0 : 0.0;

    if (parameter.type == ParamType::Choice && !parameter.choices.empty())
    {
        const auto maxIndex =
            static_cast<double>(parameter.choices.size() - 1);
        if (maxIndex <= 0.0)
            return 0.0;
        return std::clamp(std::round(plainValue) / maxIndex, 0.0, 1.0);
    }

    if (parameter.type == ParamType::Stepped && parameter.steps > 1)
    {
        const auto maxStep = static_cast<double>(parameter.steps - 1);
        return std::clamp(std::round(plainValue) / maxStep, 0.0, 1.0);
    }

    if (parameter.maxValue == parameter.minValue)
        return 0.0;

    return std::clamp(
        (plainValue - parameter.minValue) /
            (parameter.maxValue - parameter.minValue),
        0.0,
        1.0);
}

inline double normalizedToPlain(
    const ::Parameter& parameter,
    double normalizedValue)
{
    const auto clamped = std::clamp(normalizedValue, 0.0, 1.0);

    if (parameter.type == ParamType::Bool)
        return clamped >= 0.5 ? 1.0 : 0.0;

    if (parameter.type == ParamType::Choice && !parameter.choices.empty())
        return std::round(
            clamped * static_cast<double>(parameter.choices.size() - 1));

    if (parameter.type == ParamType::Stepped && parameter.steps > 1)
        return std::round(
            clamped * static_cast<double>(parameter.steps - 1));

    const auto plain =
        parameter.minValue +
        clamped * (parameter.maxValue - parameter.minValue);
    if (parameter.type == ParamType::Stepped)
        return std::round(plain);

    return plain;
}

} // namespace Steinberg::SingularityVst3
