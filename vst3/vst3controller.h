//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "IParameterProvider.h"
#include "vst3parameterhelpers.h"
#include "pluginterfaces/vst/vsttypes.h"
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

    void addSingularityParameterGroup(const ::ParameterGroup& group)
    {
        Vst::String128 name{};
        SingularityVst3::copyAsciiToString128(group.name, name);
        addUnit(new Vst::Unit(
            name,
            static_cast<Vst::UnitID>(group.id),
            static_cast<Vst::UnitID>(group.parentId)));
    }

//------------------------------------------------------------------------
    void addSingularityParameter(const ::Parameter& parameter)
    {
        using namespace SingularityVst3;

        Vst::String128 title{};
        Vst::String128 shortTitle{};
        Vst::String128 units{};
        copyAsciiToString128(parameter.name, title);
        copyAsciiToString128(parameter.shortName, shortTitle);
        copyAsciiToString128(parameter.units, units);

        auto* unitString = parameter.units.empty() ? nullptr : units;
        auto* shortTitleString = parameter.shortName.empty() ? nullptr : shortTitle;
        const auto flags = flagsFor(parameter);
        const auto unitId = static_cast<Vst::UnitID>(parameter.unitId);

        if (parameter.type == ParamType::Choice || !parameter.choices.empty())
        {
            auto* choiceParameter = new Vst::StringListParameter(
                title,
                parameter.id,
                unitString,
                flags,
                unitId,
                shortTitleString);

            for (const auto& choice : parameter.choices)
            {
                Vst::String128 choiceTitle{};
                copyAsciiToString128(choice, choiceTitle);
                choiceParameter->appendString(choiceTitle);
            }

            choiceParameter->setNormalized(plainToNormalized(parameter, parameter.defaultValue));
            parameters.addParameter(choiceParameter);
            return;
        }

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
                unitId,
                shortTitleString));
            return;
        }

        parameters.addParameter(title, unitString, stepCountFor(parameter),
            plainToNormalized(parameter, parameter.defaultValue),
            flags, parameter.id, unitId, shortTitleString);
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

        const auto normalizedValue = parameter->toNormalized(value);
        beginEdit(id);
        EditControllerEx1::setParamNormalized(id, normalizedValue);
        performEdit(id, normalizedValue);
        endEdit(id);
    }

protected:
};

//------------------------------------------------------------------------
} // namespace Steinberg