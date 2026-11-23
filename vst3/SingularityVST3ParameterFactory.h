#pragma once

#include "SingularityParameter.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "public.sdk/source/vst/utility/stringconvert.h"

inline Steinberg::Vst::Parameter* createVst3Parameter(
    const Parameter& parameter)
{
    namespace Vst = Steinberg::Vst;

    const auto title = Vst::StringConvert::convert(parameter.name);
    const auto flags = parameter.direction == ParameterDirection::output ? Vst::ParameterInfo::kIsReadOnly : Vst::ParameterInfo::kCanAutomate;
    const auto unit = Vst::StringConvert::convert(parameter.unit);

    return std::visit(
        [&](const auto& definition) -> Vst::Parameter*
        {
            using Type = std::remove_cvref_t<decltype(definition)>;


            if constexpr (std::is_same_v<Type, ContinuousParameter>)
            {
                return new Vst::RangeParameter(
                    title.c_str(),
                    parameter.id,
                    unit.c_str(),
                    definition.minValue,
                    definition.maxValue,
                    definition.defaultValue,
                    Vst::kStepCountContinuous,
                    flags);
            }
            else if constexpr (std::is_same_v<Type, SteppedParameter>)
            {
                return new Vst::RangeParameter(
                    title.c_str(),
                    parameter.id,
                    unit.c_str(),
                    definition.minValue,
                    definition.maxValue,
                    definition.defaultValue,
                    definition.maxValue - definition.minValue, // stepCount: stepped
                    flags);
            }
            else if constexpr (std::is_same_v<Type, BooleanParameter>)
            {
                return new Vst::Parameter(
                    title.c_str(),
                    parameter.id,
                    unit.c_str(),
                    definition.defaultValue ? 1.0 : 0.0,
                    Vst::kStepCountToggle,
                    flags);
            }
            else if constexpr (std::is_same_v<Type, ChoiceParameter>)
            {
                auto* result = new Vst::StringListParameter(title.c_str(), parameter.id, unit.c_str(), flags | Vst::ParameterInfo::kIsList);

                for (const auto& choice : definition.choices)
                    result->appendString(Vst::StringConvert::convert(choice).c_str());

                const auto initial = result->toNormalized(definition.defaultIndex);

                result->getInfo().defaultNormalizedValue = initial;
                result->setNormalized(initial);

                return result;
            }
        },
        parameter.definition);
}
