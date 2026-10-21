//------------------------------------------------------------------------
// Copyright(c) 2026 My Plug-in Company.
//------------------------------------------------------------------------
#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/utility/stringconvert.h"

#include "controller.h"
#include "plugincids.h"
#include "SingularityVST3Editor.h"
#include "SingularityVST3ParameterFactory.h"
#include PLUGIN_CLASS_HEADER



using namespace Steinberg;


//------------------------------------------------------------------------
// VST3Controller Implementation
//------------------------------------------------------------------------
tresult PLUGIN_API VST3Controller::initialize (FUnknown* context)
{
    // Here the Plug-in will be instantiated

    //---do not forget to call parent ------
    tresult result = EditControllerEx1::initialize (context);
    if (result != kResultOk)
    {
        return result;
    }

    // Here you could register some parameters

    parameters.addParameter (STR16 ("Bypass"), nullptr, 1, 0,
                            Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass,
                            Steinberg::Vst::kMaxParamId);

    for(const auto& definition : PLUGIN_CLASS::getParameters())
    {
        parameters.addParameter(createVst3Parameter(definition));
    }

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
    // Here you get the state of the component (Processor part)
    if (!state)
        return kResultFalse;

    IBStreamer streamer (state, kLittleEndian);

    uint32 numParams;
    if(!streamer.readInt32u(numParams))
        return kResultFalse;

    Vst::ParamID paramID;
    Vst::ParamValue value;
    
    for(int i = 0; i < numParams; ++i)
    {
        if(!streamer.readInt32u(paramID))
            break;
        if(!streamer.readDouble(value))
            break;
        if(auto parameter = parameters.getParameter(paramID))
            parameter->setNormalized(value);
    }

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

tresult PLUGIN_API VST3Controller::notify (Steinberg::Vst::IMessage* message)
{
    if(!message)
        return kInvalidArgument;

    if(!std::strcmp(message->getMessageID(), "Singularity.ActionResult"))
    {
        ActionValue value = std::monostate{};

        const void* nameData{nullptr};
        uint32 nameSize{0};

        int64 intValue{0};
        double floatValue{0.0};
        const void* stringData{nullptr};
        uint32 stringSize{0};

        auto attributes = message->getAttributes();

        attributes->getBinary("name", nameData, nameSize);


        if(attributes->getInt("boolValue", intValue) == kResultTrue)
            value = intValue != 0;
        else if(attributes->getInt("intValue", intValue) == kResultTrue)
            value = intValue;
        else if(attributes->getFloat("floatValue", floatValue) == kResultTrue)
            value = floatValue;
        else if(attributes->getBinary("stringValue", stringData, stringSize) == kResultTrue)
            value = std::string(static_cast<const char*>(stringData), stringSize);
            
        const std::string_view name{static_cast<const char*>(nameData), nameSize};
        editor_->setData(name, value);

        return kResultTrue;
    }
    return EditControllerEx1::notify(message);
}

tresult PLUGIN_API VST3Controller::setParamNormalized (Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value)
{
    return EditControllerEx1::setParamNormalized(tag, value);
}

//------------------------------------------------------------------------
IPlugView* PLUGIN_API VST3Controller::createView (FIDString name)
{
    // Here the Host wants to open your editor (if you have one)
    if (FIDStringsEqual (name, Vst::ViewType::kEditor))
    {
        // create your editor here and return a IPlugView ptr of it
        editor_ = new SingularityVST3Editor(this);
        return editor_;
    }
    return nullptr;
}

void VST3Controller::editorDestroyed (Steinberg::Vst::EditorView* editor)
{
    if(editor_ == editor)
        editor_ = nullptr;
}


double VST3Controller::getParameter(unsigned int id)
{
    auto* parameter = getParameterObject(id);
    if(!parameter)
        return 0.0;

    return parameter->toPlain(getParamNormalized(id));
}

void VST3Controller::setParameter(unsigned int id, double value)
{
    auto* parameter = getParameterObject(id);
    if(!parameter)
        return;

    const double normalized = parameter->toNormalized(value);

    beginEdit(id);
    setParamNormalized(id, normalized);
    performEdit(id, normalized);
    endEdit(id);
}

void VST3Controller::sendAction(std::string_view name, const ActionValue& value)
{
    auto message = owned(allocateMessage());
    if(!message)
        return;

    message->setMessageID("Singularity.Action");
    message->getAttributes()->setBinary("name", name.data(), name.size());

    auto attributes = message->getAttributes();

    std::visit(
        [attributes](const auto& payload)
        {
            using T = std::decay_t<decltype(payload)>;

            if constexpr (std::is_same_v<T, bool>)
                attributes->setInt("boolValue", payload ? 1 : 0);
            else if constexpr (std::is_same_v<T, std::int64_t>)
                attributes->setInt("intValue", payload);
            else if constexpr (std::is_same_v<T, double>)
                attributes->setFloat("floatValue", payload);
            else if constexpr (std::is_same_v<T, std::string>)
                attributes->setBinary("stringValue", payload.data(), payload.size()); 
        }, 
        value);    

    ComponentBase::sendMessage(message);
}
//------------------------------------------------------------------------
