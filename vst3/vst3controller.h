//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "IParameterProvider.h"
#include <limits>

namespace Steinberg {

// Bridges VST3 parameter system to IParameterProvider
class VST3ParameterAdapter : public IParameterProvider
{
public:
    VST3ParameterAdapter(Vst::EditController* c) : _controller(c) {}
    double getParameter(int id) const override {
        if (!_controller->getParameterObject(id))
            return std::numeric_limits<double>::quiet_NaN();
        return _controller->getParamNormalized(id);
    }
    void setParameter(int id, double value) override
    {
        _controller->beginEdit(id);
        _controller->setParamNormalized(id, value);
        _controller->performEdit(id, value);
        _controller->endEdit(id);
    }
private:
    Vst::EditController* _controller;
};
//------------------------------------------------------------------------
//  HelloWorldController
//------------------------------------------------------------------------
class HelloWorldController : public Steinberg::Vst::EditControllerEx1
{
public:
//------------------------------------------------------------------------
	HelloWorldController () = default;
	~HelloWorldController () SMTG_OVERRIDE = default;

    // Create function
	static Steinberg::FUnknown* createInstance (void* /*context*/)
	{
		return (Steinberg::Vst::IEditController*)new HelloWorldController;
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

protected:

private:
    VST3ParameterAdapter _paramAdapter{this};
};

//------------------------------------------------------------------------
} // namespace Steinberg