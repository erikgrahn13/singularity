//------------------------------------------------------------------------
// Copyright(c) 2022 Steinberg Media Technologies GmbH.
//------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "public.sdk/source/vst/utility/dataexchange.h"
#include "public.sdk/source/vst/utility/stringconvert.h"
#include "AudioDataExchange.h"
#include "Vst3ParameterSupport.h"
#include "Vst3ProgramData.h"
#include "Vst3ProgramLayout.h"
#include "Vst3ProgramModel.h"
#include "IParameterProvider.h"
#include "pluginterfaces/vst/vsttypes.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <optional>

namespace Steinberg {

//------------------------------------------------------------------------
//  VST3Controller
//------------------------------------------------------------------------
class VST3Controller : public Steinberg::Vst::EditControllerEx1,
                       public Steinberg::Vst::IDataExchangeReceiver,
                       public IParameterProvider,
                       public Singularity::AudioDataExchange::IDataSink
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
	Steinberg::tresult PLUGIN_API notify (Steinberg::Vst::IMessage* message) SMTG_OVERRIDE;
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
    Steinberg::tresult PLUGIN_API getUnitByBus (
        Steinberg::Vst::MediaType type,
        Steinberg::Vst::BusDirection direction,
        Steinberg::int32 busIndex,
        Steinberg::int32 channel,
        Steinberg::Vst::UnitID& unitId) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setUnitProgramData (
        Steinberg::int32 listOrUnitId,
        Steinberg::int32 programIndex,
        Steinberg::IBStream* data) SMTG_OVERRIDE;

 	//---Interface---------
	DEFINE_INTERFACES
		DEF_INTERFACE (Vst::IDataExchangeReceiver)
	END_DEFINE_INTERFACES (EditControllerEx1)
    DELEGATE_REFCOUNT (EditControllerEx1)

    void addSingularityParameter(
        const ::Parameter& parameter,
        Vst::UnitID unitId)
    {
        Vst::String128 title{};
        Vst::String128 shortTitle{};
        Vst::String128 units{};
        copyUtf8ToString128(parameter.name, title);
        copyUtf8ToString128(parameter.shortName, shortTitle);
        copyUtf8ToString128(parameter.units, units);

        auto* unitString = parameter.units.empty() ? nullptr : units;
        auto* shortTitleString = parameter.shortName.empty() ? nullptr : shortTitle;
        const auto flags = flagsFor(parameter);
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
                unitId,
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
                unitId,
                shortTitleString));
            return;
        }

        parameters.addParameter(title, unitString, stepCountFor(parameter),
            SingularityVst3::plainToNormalized(
                parameter, parameter.defaultValue),
            flags, parameter.id, unitId, shortTitleString);
    }

    void PLUGIN_API queueOpened (Steinberg::Vst::DataExchangeUserContextID userContextID, Steinberg::uint32 blockSize, Steinberg::TBool& dispatchOnBackgroundThread) SMTG_OVERRIDE;
    void PLUGIN_API queueClosed (Steinberg::Vst::DataExchangeUserContextID userContextID) SMTG_OVERRIDE;
    void PLUGIN_API onDataExchangeBlocksReceived (Steinberg::Vst::DataExchangeUserContextID userContextID, Steinberg::uint32 numBlocks, Steinberg::Vst::DataExchangeBlock* blocks, Steinberg::TBool onBackgroundThread) SMTG_OVERRIDE;

    Singularity::AudioDataExchange::AudioDataQueue& audioDataQueue() { return audioDataQueue_; }
    void pushAudioDataBlock(const Singularity::AudioDataExchange::AudioDataBlock& block) override { audioDataQueue_.pushAudioDataBlock(block); }

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
        setParamNormalized(id, normalizedValue);
        performEdit(id, normalizedValue);
        endEdit(id);
    }

private:
    struct ControllerProgramBank
        : SingularityVst3::ProgramModelBank
    {
        std::vector<std::optional<SingularityVst3::ProgramData>>
            programOverrides;
        int32 currentProgram = 0;
    };

    bool initializeProgramBanks();
    bool applyProgram(
        ControllerProgramBank& bank,
        int32 programIndex,
        bool notifyHost);
    int32 programIndex(
        const ControllerProgramBank& bank,
        Vst::ParamValue normalizedValue) const;
    ControllerProgramBank* findProgramBank(Vst::ProgramListID listId);
    ControllerProgramBank* findProgramBankBySelector(Vst::ParamID selectorId);
    Vst::UnitID unitIdForParameter(const ::Parameter& parameter) const;

    static bool copyUtf8ToString128(
        const std::string& source,
        Vst::String128 target)
    {
        return Vst::StringConvert::convert(source, target);
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


    Vst::DataExchangeReceiverHandler dataExchange_ {this};
    Singularity::AudioDataExchange::AudioDataQueue audioDataQueue_;
    std::vector<::Vst3ProgramUnit> programUnits_;
    std::vector<ControllerProgramBank> programBanks_;

protected:
};

//------------------------------------------------------------------------
} // namespace Steinberg
