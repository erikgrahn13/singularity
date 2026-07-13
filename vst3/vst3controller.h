//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "IParameterProvider.h"
#include "pluginterfaces/vst/vsttypes.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace Steinberg {

//------------------------------------------------------------------------
//  VST3Controller
//------------------------------------------------------------------------
class VST3Controller : public Steinberg::Vst::EditControllerEx1,
                       public IParameterProvider
{
public:
//------------------------------------------------------------------------
	VST3Controller () = default;
	~VST3Controller () SMTG_OVERRIDE = default;

    // Create function
	static Steinberg::FUnknown* createInstance (void* /*context*/)
	{
		return (Steinberg::Vst::IEditController*)new VST3Controller;
	}

	// IPluginBase
	Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API terminate () SMTG_OVERRIDE;

	// EditController
	Steinberg::tresult PLUGIN_API setComponentState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::IPlugView* PLUGIN_API createView (Steinberg::FIDString name) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setParamNormalized (Steinberg::Vst::ParamID tag,
                                                      Steinberg::Vst::ParamValue value) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getParamStringByValue (Steinberg::Vst::ParamID tag,
                                                         Steinberg::Vst::ParamValue valueNormalized,
                                                         Steinberg::Vst::String128 string) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getParamValueByString (Steinberg::Vst::ParamID tag,
                                                         Steinberg::Vst::TChar* string,
                                                         Steinberg::Vst::ParamValue& valueNormalized) SMTG_OVERRIDE;

 	//---Interface---------
	DEFINE_INTERFACES
		// Here you can add more supported VST3 interfaces
		// DEF_INTERFACE (Vst::IXXX)
	END_DEFINE_INTERFACES (EditController)
    DELEGATE_REFCOUNT (EditController)

    void addSingularityParameter(const ::Parameter& parameter)
    {
        Vst::String128 title{};
        Vst::String128 shortTitle{};
        Vst::String128 units{};
        copyAsciiToString128(parameter.name, title);
        copyAsciiToString128(parameter.shortName, shortTitle);
        copyAsciiToString128(parameter.units, units);

        auto* unitString = parameter.units.empty() ? nullptr : units;
        auto* shortTitleString = parameter.shortName.empty() ? nullptr : shortTitle;
        const auto flags = flagsFor(parameter);
        const auto groupId = static_cast<Vst::UnitID>(parameter.groupId);

        if (parameter.type == ParamType::Float)
        {
            parameters.addParameter(new Vst::RangeParameter(
                title,
                parameter.id,
                unitString,
                parameter.minValue,
                parameter.maxValue,
                parameter.defaultValue,
                stepCountFor(parameter),
                flags,
                groupId,
                shortTitleString));
            return;
        }

        if (parameter.type == ParamType::Choice && !parameter.choices.empty())
        {
            const auto maxIndex = static_cast<double>(parameter.choices.size() - 1);
            parameters.addParameter(new Vst::RangeParameter(
                title,
                parameter.id,
                unitString,
                0.0,
                maxIndex,
                std::clamp(std::round(parameter.defaultValue), 0.0, maxIndex),
                stepCountFor(parameter),
                flags,
                groupId,
                shortTitleString));
            return;
        }

        if (parameter.type == ParamType::Stepped && parameter.steps > 1)
        {
            const auto maxStep = static_cast<double>(parameter.steps - 1);
            parameters.addParameter(new Vst::RangeParameter(
                title,
                parameter.id,
                unitString,
                0.0,
                maxStep,
                std::clamp(std::round(parameter.defaultValue), 0.0, maxStep),
                stepCountFor(parameter),
                flags,
                groupId,
                shortTitleString));
            return;
        }

        parameters.addParameter(title, unitString, stepCountFor(parameter),
            plainToNormalized(parameter, parameter.defaultValue),
            flags, parameter.id, groupId, shortTitleString);
    }

    // IParameterProvider
    double getParameter(int id) override
    {
        auto* parameter = getParameterObject(id);
        if (!parameter)
            return std::numeric_limits<double>::quiet_NaN();

        return parameter->toPlain(getParamNormalized(id));
    }
    void setParameter(int id, double value) override
    {
        auto* parameter = getParameterObject(id);
        if (!parameter)
            return;
		if (parameter->getInfo().flags & Vst::ParameterInfo::kIsReadOnly)
			return;

        const auto normalizedValue = parameter->toNormalized(value);
        beginEdit(id);
        EditControllerEx1::setParamNormalized(id, normalizedValue);
        performEdit(id, normalizedValue);
        endEdit(id);
    }

private:
    static void copyAsciiToString128(const std::string& source, Vst::String128 target)
    {
        for (int i = 0; i < 127 && i < static_cast<int>(source.size()); ++i)
            target[i] = source[static_cast<std::size_t>(i)];
    }

    static int32 stepCountFor(const ::Parameter& parameter)
    {
        switch (parameter.type)
        {
            case ParamType::Bool:
                return 1;
            case ParamType::Choice:
                return static_cast<int32>(parameter.choices.empty() ? std::max(1, parameter.steps - 1) : parameter.choices.size() - 1);
            case ParamType::Stepped:
                return std::max<int32>(1, parameter.steps - 1);
            case ParamType::Float:
            default:
                return 0;
        }
    }

    static int32 flagsFor(const ::Parameter& parameter)
    {
        int32 flags = 0;
        if (parameter.automatable)
            flags |= Vst::ParameterInfo::kCanAutomate;
        if (parameter.readOnly)
            flags |= Vst::ParameterInfo::kIsReadOnly;
        if (parameter.wrapAround)
            flags |= Vst::ParameterInfo::kIsWrapAround;
        if (parameter.type == ParamType::Choice || !parameter.choices.empty())
            flags |= Vst::ParameterInfo::kIsList;
        return flags;
    }


    static double plainToNormalized(const ::Parameter& parameter, double plainValue)
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

        if (parameter.type == ParamType::Stepped && parameter.steps > 1)
        {
            const auto maxStep = static_cast<double>(parameter.steps - 1);
            return std::clamp(std::round(plainValue) / maxStep, 0.0, 1.0);
        }

        if (parameter.maxValue == parameter.minValue)
            return 0.0;

        return std::clamp((plainValue - parameter.minValue) /
            (parameter.maxValue - parameter.minValue), 0.0, 1.0);
    }

protected:
};

//------------------------------------------------------------------------
} // namespace Steinberg
