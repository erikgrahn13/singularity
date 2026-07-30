#pragma once

#include "Vst3ParameterSupport.h"
#include "Vst3ProgramData.h"
#include "Vst3ProgramLayout.h"
#include "pluginterfaces/vst/ivstunits.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace Steinberg::SingularityVst3 {

struct ProgramModelBank
{
    ::ProgramCollection definition;
    int32 unitId = Vst::kRootUnitId;
    Vst::ProgramListID listId = Vst::kNoProgramListId;
    Vst::ParamID selectorId = Vst::kNoParamId;
    std::vector<ProgramData> programs;
};

struct ProgramModel
{
    std::vector<::Vst3ProgramUnit> units;
    std::vector<ProgramModelBank> banks;
};

template<typename PluginType>
std::optional<ProgramModel> buildProgramModel()
{
    ProgramModel model;
    if constexpr (!HasVst3ProgramLists<PluginType>)
    {
        return model;
    }
    else
    {
        if constexpr (HasVst3ProgramUnits<PluginType>)
            for (const auto& unit : PluginType::getVst3ProgramUnits())
                model.units.push_back(unit);
        if (!validateVst3ProgramUnits(model.units))
            return std::nullopt;

        std::vector<::Vst3ProgramListBinding> bindings;
        for (const auto& binding : PluginType::getVst3ProgramListBindings())
            bindings.push_back(binding);

        const auto pluginParameters = PluginType::getParameters();
        std::unordered_set<Vst::ParamID> parameterIds;
        for (const auto& parameter : pluginParameters)
            if (parameter.id >= Vst::kMaxParamId ||
                !parameterIds.insert(parameter.id).second)
                return std::nullopt;

        std::unordered_set<int32> unitIds {Vst::kRootUnitId};
        for (const auto& unit : model.units)
            unitIds.insert(unit.id);

        std::unordered_set<std::string> collectionIds;
        std::unordered_set<Vst::ProgramListID> listIds;
        std::unordered_set<int32> unitsWithBanks;
        std::unordered_set<unsigned int> parametersInCollections;
        std::size_t collectionCount = 0;
        for (const auto& definition : PluginType::getProgramCollections())
        {
            ++collectionCount;
            if (definition.id.empty() || definition.name.empty() ||
                definition.programs.empty() ||
                definition.programs.size() >
                    static_cast<std::size_t>(
                        std::numeric_limits<int32>::max() - 1) ||
                definition.parameterIds.size() >
                    static_cast<std::size_t>(
                        kMaximumSerializedParameters) ||
                !collectionIds.insert(definition.id).second)
                return std::nullopt;

            const auto binding = std::find_if(
                bindings.begin(),
                bindings.end(),
                [&definition](const auto& candidate)
                {
                    return candidate.collectionId == definition.id;
                });
            if (binding == bindings.end() ||
                !unitIds.contains(binding->unitId) ||
                !unitsWithBanks.insert(binding->unitId).second)
                return std::nullopt;

            ProgramModelBank bank;
            bank.definition = definition;
            bank.unitId = binding->unitId;
            bank.listId = programListId(definition.id);
            bank.selectorId = bank.listId;
            if (!listIds.insert(bank.listId).second)
                return std::nullopt;
            for (const auto& parameter : pluginParameters)
                if (parameter.id == bank.selectorId ||
                    parameter.id == Vst::kMaxParamId)
                    return std::nullopt;

            std::unordered_set<unsigned int> collectionParameterIds;
            for (const auto id : definition.parameterIds)
            {
                const auto parameter = std::find_if(
                    pluginParameters.begin(),
                    pluginParameters.end(),
                    [id](const auto& candidate)
                    {
                        return candidate.id == id;
                    });
                if (parameter == pluginParameters.end() ||
                    parameter->readOnly ||
                    !collectionParameterIds.insert(id).second ||
                    !parametersInCollections.insert(id).second)
                    return std::nullopt;
            }

            std::unordered_set<std::string> programIds;
            for (const auto& program : definition.programs)
            {
                if (program.id.empty() || program.name.empty() ||
                    program.data.size() >
                        static_cast<std::size_t>(
                            kMaximumSerializedPayloadBytes) ||
                    !programIds.insert(program.id).second)
                    return std::nullopt;
                if constexpr (!HandlesProgramData<PluginType>)
                    if (!program.data.empty())
                        return std::nullopt;

                ProgramData data;
                data.payload = program.data;
                for (const auto id : definition.parameterIds)
                {
                    const auto parameter = std::find_if(
                        pluginParameters.begin(),
                        pluginParameters.end(),
                        [id](const auto& candidate)
                        {
                            return candidate.id == id;
                        });
                    if (parameter == pluginParameters.end())
                        return std::nullopt;
                    data.parameters.emplace_back(
                        parameter->id,
                        plainToNormalized(
                            *parameter, parameter->defaultValue));
                }

                std::unordered_set<int> changedParameterIds;
                for (const auto& change : program.parameters)
                {
                    if (!std::isfinite(change.value) ||
                        !changedParameterIds.insert(change.id).second)
                        return std::nullopt;

                    const auto stored = std::find_if(
                        data.parameters.begin(),
                        data.parameters.end(),
                        [&change](const auto& candidate)
                        {
                            return static_cast<int>(candidate.first) ==
                                change.id;
                        });
                    if (stored == data.parameters.end())
                        return std::nullopt;

                    const auto parameter = std::find_if(
                        pluginParameters.begin(),
                        pluginParameters.end(),
                        [&stored](const auto& candidate)
                        {
                            return candidate.id == stored->first;
                        });
                    if (parameter == pluginParameters.end())
                        return std::nullopt;
                    stored->second =
                        plainToNormalized(*parameter, change.value);
                }
                bank.programs.push_back(std::move(data));
                auto& storedDefinition =
                    bank.definition.programs[bank.programs.size() - 1];
                storedDefinition.parameters.clear();
                storedDefinition.data.clear();
            }
            model.banks.push_back(std::move(bank));
        }

        if (bindings.size() != collectionCount)
            return std::nullopt;
        return model;
    }
}

} // namespace Steinberg::SingularityVst3
