#include "vst3controller.h"
#include "plugincids.h"
#include "SingularityView.h"
#include "Vst3ComponentState.h"
#include "Vst3ParameterSupport.h"
#include "Vst3ProgramData.h"
#include "Vst3ProgramModel.h"
#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/vstpresetkeys.h"
#include "SingularityPlugin.h"
#include PLUGIN_CLASS_HEADER
#include <cmath>

namespace Steinberg {

//------------------------------------------------------------------------
// VST3Controller Implpementation
//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::initialize (FUnknown* context)
{
	//---do not forget to call parent ------
	tresult result = EditControllerEx1::initialize (context);
	if (result != kResultOk)
	{
		return result;
	}

	parameters.addParameter (STR16 ("Bypass"), nullptr, 1, 0,
								Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass,
								Steinberg::Vst::kMaxParamId);

	if (!initializeProgramBanks())
		return kInvalidArgument;

	for (auto& p : PLUGIN_CLASS::getParameters ())
		addSingularityParameter (p, unitIdForParameter(p));

	for (auto& bank : programBanks_)
		applyProgram(bank, 0, false);

	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::terminate ()
{
	// Here the Plug-in will be de-instantiated, last possibility to remove some memory!

	//---do not forget to call parent ------
	return EditControllerEx1::terminate ();
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::setComponentState (IBStream* state)
{
	if (!state)
		return kResultFalse;

	SingularityVst3::ComponentState restored;
	const auto pluginParameters = PLUGIN_CLASS::getParameters();
	if (!SingularityVst3::readComponentState(
			state, pluginParameters, restored))
		return kResultFalse;

	setParamNormalized(
		Steinberg::Vst::kMaxParamId, restored.bypass ? 1.0 : 0.0);

	for (auto& bank : programBanks_)
	{
		for (auto& program : bank.programOverrides)
			program.reset();
		auto* selector = getParameterObject(bank.selectorId);
		if (selector)
			EditControllerEx1::setParamNormalized(
				bank.selectorId, selector->toNormalized(0));
		applyProgram(bank, 0, false);
	}

	for (auto& saved : restored.modifiedPrograms)
	{
		auto* bank = findProgramBank(saved.listId);
		if (!bank || saved.programIndex < 0 ||
			saved.programIndex >= static_cast<int32>(bank->programs.size()))
			continue;

		auto updated =
			bank->programs[static_cast<std::size_t>(saved.programIndex)];
		for (const auto& [id, value] : saved.data.parameters)
		{
			const auto parameter = std::find_if(
				updated.parameters.begin(),
				updated.parameters.end(),
				[id](const auto& candidate)
				{
					return candidate.first == id;
				});
			if (parameter == updated.parameters.end())
				continue;
			parameter->second = value;
		}
		updated.payload = std::move(saved.data.payload);
		bank->programOverrides[
			static_cast<std::size_t>(saved.programIndex)] =
				std::move(updated);
	}

	for (const auto& selection : restored.programSelections)
	{
		auto* bank = selection.listId == Vst::kNoProgramListId
			? (programBanks_.empty() ? nullptr : &programBanks_.front())
			: findProgramBank(selection.listId);
		if (!bank || selection.programIndex < 0 ||
			selection.programIndex >=
				static_cast<int32>(bank->programs.size()))
			continue;

		auto* selector = getParameterObject(bank->selectorId);
		if (!selector)
			continue;
		EditControllerEx1::setParamNormalized(
			bank->selectorId,
			selector->toNormalized(selection.programIndex));
		applyProgram(*bank, selection.programIndex, false);
	}

	// Program selection loads a bank entry into working memory. Reapply the
	// saved working values afterwards so edits survive project restoration.
	for (const auto& [id, value] : restored.parameterValues)
		setParamNormalized(id, value);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::setState (IBStream* state)
{
	// Here you get the state of the controller

	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::getState (IBStream* state)
{
	// Here you are asked to deliver the state of the controller (if needed)
	// Note: the real state of your plug-in is saved in the processor

	return kResultTrue;
}

//------------------------------------------------------------------------
IPlugView* PLUGIN_API VST3Controller::createView (FIDString name)
{
	if (FIDStringsEqual (name, Vst::ViewType::kEditor))
	{
        // Default editor size — host may resize after creation
        return new SingularityView(this);
	}
	return nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::setParamNormalized (Vst::ParamID tag, Vst::ParamValue value)
{
	// called by host to update your parameters
	tresult result = EditControllerEx1::setParamNormalized (tag, value);
	if (result == kResultOk)
	{
		if (auto* bank = findProgramBankBySelector(tag))
		{
			const auto index = programIndex(*bank, value);
			if (index >= 0)
				applyProgram(*bank, index, true);
		}
	}
	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::getParamStringByValue (Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string)
{
	// called by host to get a string for given normalized value of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamStringByValue (tag, valueNormalized, string);
}

//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::getParamValueByString (Vst::ParamID tag, Vst::TChar* string, Vst::ParamValue& valueNormalized)
{
	// called by host to get a normalized value from a string representation of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamValueByString (tag, string, valueNormalized);
}

tresult PLUGIN_API VST3Controller::notify (Vst::IMessage* message)
{
    if (dataExchange_.onMessage(message))
        return kResultTrue;
    return EditControllerEx1::notify(message);
}

bool VST3Controller::initializeProgramBanks()
{
	programBanks_.clear();
	auto model = SingularityVst3::buildProgramModel<PLUGIN_CLASS>();
	if (!model)
		return false;
	programUnits_ = std::move(model->units);
	for (auto& bank : model->banks)
	{
		ControllerProgramBank runtime;
		static_cast<SingularityVst3::ProgramModelBank&>(runtime) =
			std::move(bank);
		runtime.programOverrides.resize(runtime.programs.size());
		programBanks_.push_back(std::move(runtime));
	}

	auto listForUnit = [&] (int32 unitId)
	{
		for (const auto& bank : programBanks_)
			if (bank.unitId == unitId)
				return bank.listId;
		return Vst::kNoProgramListId;
	};

	Vst::String128 rootName {};
	if (!copyUtf8ToString128("Root", rootName))
		return false;
	addUnit(new Vst::Unit(
		rootName,
		Vst::kRootUnitId,
		Vst::kNoParentUnitId,
		listForUnit(Vst::kRootUnitId)));
	for (const auto& unit : programUnits_)
	{
		Vst::String128 unitName {};
		if (!copyUtf8ToString128(unit.name, unitName))
			return false;
		addUnit(new Vst::Unit(
			unitName,
			unit.id,
			unit.parentId,
			listForUnit(unit.id)));
	}

	for (auto& bank : programBanks_)
	{
		Vst::String128 listName {};
		if (!copyUtf8ToString128(bank.definition.name, listName))
			return false;
		auto* programList = new Vst::ProgramList(
			listName,
			bank.listId,
			bank.unitId);
		for (const auto& program : bank.definition.programs)
		{
			Vst::String128 programName {};
			if (!copyUtf8ToString128(program.name, programName))
			{
				programList->release();
				return false;
			}
			const auto index = programList->addProgram(programName);
			if constexpr (PLUGIN_CLASS::isInstrument)
			{
				if (!program.category.empty())
				{
					Vst::String128 category {};
					if (!copyUtf8ToString128(
							program.category, category))
					{
						programList->release();
						return false;
					}
					programList->setProgramInfo(
						index,
						Vst::PresetAttributes::kInstrument,
						category);
				}
			}
		}
		if (!addProgramList(programList))
		{
			programList->release();
			return false;
		}

		auto* selector = new Vst::StringListParameter(
			listName,
			bank.selectorId,
			nullptr,
			Vst::ParameterInfo::kIsList |
				Vst::ParameterInfo::kIsProgramChange,
			bank.unitId);
		for (const auto& program : bank.definition.programs)
		{
			Vst::String128 programName {};
			if (!copyUtf8ToString128(program.name, programName))
			{
				selector->release();
				return false;
			}
			selector->appendString(programName);
		}
		parameters.addParameter(selector);
	}
	return true;
}

int32 VST3Controller::programIndex(
	const ControllerProgramBank& bank,
	Vst::ParamValue normalizedValue) const
{
	if (bank.programs.empty())
		return -1;
	const auto maxIndex = static_cast<double>(bank.programs.size() - 1);
	return static_cast<int32>(std::clamp(
		std::round(std::clamp(normalizedValue, 0.0, 1.0) * maxIndex),
		0.0,
		maxIndex));
}

bool VST3Controller::applyProgram(
	ControllerProgramBank& bank,
	int32 index,
	bool notifyHost)
{
	if (index < 0 || index >= static_cast<int32>(bank.programs.size()))
		return false;

	bank.currentProgram = index;
	const auto programIndex = static_cast<std::size_t>(index);
	const auto& program = bank.programOverrides[programIndex]
		? *bank.programOverrides[programIndex]
		: bank.programs[programIndex];
	for (const auto& [id, value] :
		program.parameters)
	{
		EditControllerEx1::setParamNormalized(id, value);
	}

	if (notifyHost && componentHandler)
		componentHandler->restartComponent(Vst::kParamValuesChanged);
	return true;
}

VST3Controller::ControllerProgramBank* VST3Controller::findProgramBank(
	Vst::ProgramListID listId)
{
	for (auto& bank : programBanks_)
		if (bank.listId == listId)
			return &bank;
	return nullptr;
}

VST3Controller::ControllerProgramBank*
VST3Controller::findProgramBankBySelector(Vst::ParamID selectorId)
{
	for (auto& bank : programBanks_)
		if (bank.selectorId == selectorId)
			return &bank;
	return nullptr;
}

Vst::UnitID VST3Controller::unitIdForParameter(
	const ::Parameter& parameter) const
{
	for (const auto& bank : programBanks_)
	{
		if (std::find(
				bank.definition.parameterIds.begin(),
				bank.definition.parameterIds.end(),
				parameter.id) != bank.definition.parameterIds.end())
			return bank.unitId;
	}
	return static_cast<Vst::UnitID>(parameter.groupId);
}

tresult PLUGIN_API VST3Controller::getUnitByBus(
	Vst::MediaType type,
	Vst::BusDirection direction,
	int32 busIndex,
	int32 channel,
	Vst::UnitID& unitId)
{
	if (type != Vst::kEvent || direction != Vst::kInput)
		return kResultFalse;

	for (const auto& unit : programUnits_)
	{
		if (unit.eventBusIndex == busIndex && unit.midiChannel == channel)
		{
			unitId = unit.id;
			return kResultTrue;
		}
	}
	if (busIndex == 0 && channel == 0)
	{
		unitId = Vst::kRootUnitId;
		return kResultTrue;
	}
	return kResultFalse;
}

tresult PLUGIN_API VST3Controller::setUnitProgramData(
	int32 listOrUnitId, int32 programIndex, IBStream* data)
{
	auto* bank = findProgramBank(listOrUnitId);
	if (!bank || programIndex < 0 ||
		programIndex >= static_cast<int32>(bank->programs.size()))
		return kInvalidArgument;

	SingularityVst3::ProgramData decoded;
	if (!SingularityVst3::readProgramData(data, decoded))
		return kResultFalse;
	if constexpr (!HandlesProgramData<PLUGIN_CLASS>)
		if (!decoded.payload.empty())
			return kInvalidArgument;

	const auto slot = static_cast<std::size_t>(programIndex);
	auto updated = bank->programOverrides[slot]
		? *bank->programOverrides[slot]
		: bank->programs[slot];
	const auto pluginParameters = PLUGIN_CLASS::getParameters();
	for (const auto& [id, value] : decoded.parameters)
	{
		auto found = false;
		for (std::size_t index = 0; index < pluginParameters.size(); ++index)
		{
				if (pluginParameters[index].id == id &&
					!pluginParameters[index].readOnly &&
					std::find(
						bank->definition.parameterIds.begin(),
						bank->definition.parameterIds.end(),
						id) != bank->definition.parameterIds.end())
			{
				for (auto& [updatedId, updatedValue] : updated.parameters)
				{
					if (updatedId == id)
					{
						updatedValue = value;
						found = true;
						break;
					}
				}
				break;
			}
		}
		if (!found)
			return kInvalidArgument;
	}
	updated.payload = std::move(decoded.payload);

	bank->programOverrides[slot] = std::move(updated);
	if (bank->currentProgram == programIndex)
		applyProgram(*bank, programIndex, true);
	return kResultTrue;
}

void PLUGIN_API VST3Controller::queueOpened (Vst::DataExchangeUserContextID userContextID, uint32, TBool& dispatchOnBackgroundThread)
{
    if (userContextID == Singularity::AudioDataExchange::kDefaultContextID)
        dispatchOnBackgroundThread = false;
}

void PLUGIN_API VST3Controller::queueClosed (Vst::DataExchangeUserContextID)
{
}

void PLUGIN_API VST3Controller::onDataExchangeBlocksReceived (Vst::DataExchangeUserContextID userContextID, uint32 numBlocks, Vst::DataExchangeBlock* blocks, TBool)
{
    if (userContextID != Singularity::AudioDataExchange::kDefaultContextID)
        return;

    for (uint32 index = 0; index < numBlocks; ++index)
        if (blocks[index].data)
            pushAudioDataBlock(*Singularity::AudioDataExchange::toAudioDataBlock(blocks[index].data));
}

//------------------------------------------------------------------------
} // namespace Steinberg
