//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "IParameterProvider.h"
#include "pluginterfaces/vst/vsttypes.h"
#include <algorithm>
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

//------------------------------------------------------------------------
    void addSingularityParameter(const ::Parameter& parameter)
    {
        Vst::String128 title{};
        copyAsciiToString128(parameter.name.c_str(), title);

        if (parameter.type == ParamType::Float)
        {
            parameters.addParameter(new Vst::RangeParameter(
                title,
                parameter.id,
                nullptr,
                parameter.minValue,
                parameter.maxValue,
                parameter.defaultValue,
                0,
                Vst::ParameterInfo::kCanAutomate));
            return;
        }

        parameters.addParameter(title, nullptr, stepCountFor(parameter),
            plainToNormalized(parameter, parameter.defaultValue),
            Vst::ParameterInfo::kCanAutomate, parameter.id);
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

private:
    static void copyAsciiToString128(const char* source, Vst::String128 target)
    {
        for (int i = 0; source[i] && i < 127; ++i)
            target[i] = source[i];
    }

    static int32 stepCountFor(const ::Parameter& parameter)
    {
        switch (parameter.type)
        {
            case ParamType::Bool:
                return 1;
            case ParamType::Stepped:
                return std::max(1, parameter.steps - 1);
            case ParamType::Float:
            default:
                return 0;
        }
    }

    static double plainToNormalized(const ::Parameter& parameter, double plainValue)
    {
        if (parameter.maxValue == parameter.minValue)
            return 0.0;

        return std::clamp((plainValue - parameter.minValue) /
            (parameter.maxValue - parameter.minValue), 0.0, 1.0);
    }

protected:
};

//------------------------------------------------------------------------
} // namespace Steinberg