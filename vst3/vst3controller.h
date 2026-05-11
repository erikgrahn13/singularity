//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "IParameterProvider.h"
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
    void addSingularityParameter(int id, const char* name, double defaultValue)
    {
        Vst::String128 title{};
        for (int i = 0; name[i] && i < 127; ++i) title[i] = name[i];
        parameters.addParameter(title, nullptr, 0, defaultValue,
            Vst::ParameterInfo::kCanAutomate, id);
    }

    // IParameterProvider
    double getParameter(int id) override
    {
        if (!getParameterObject(id))
            return std::numeric_limits<double>::quiet_NaN();
        return getParamNormalized(id);
    }
    void setParameter(int id, double value) override
    {
        beginEdit(id);
        EditControllerEx1::setParamNormalized(id, value);
        performEdit(id, value);
        endEdit(id);
    }

protected:

private:
};

//------------------------------------------------------------------------
} // namespace Steinberg