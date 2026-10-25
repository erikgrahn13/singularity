//------------------------------------------------------------------------
// Copyright(c) 2026 My Plug-in Company.
//------------------------------------------------------------------------

#pragma once

#include <functional>

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "public.sdk/source/vst/utility/dataexchange.h"

#include "SingularityPlugin.h"
#include "utilities/SingularityQueue.h"
#include "utilities/SingularityCommunication.h"
#include "dataexchange.h"

class SingularityVST3Editor;

//------------------------------------------------------------------------
//  VST3Controller
//------------------------------------------------------------------------
class VST3Controller : public Steinberg::Vst::EditControllerEx1,
                       public Steinberg::Vst::IDataExchangeReceiver
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

    //--- from IPluginBase -----------------------------------------------
    Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API terminate () SMTG_OVERRIDE;

    //--- from EditController --------------------------------------------
    Steinberg::tresult PLUGIN_API setComponentState (Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::IPlugView* PLUGIN_API createView (Steinberg::FIDString name) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API notify (Steinberg::Vst::IMessage* message) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setParamNormalized (Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value) SMTG_OVERRIDE;
	void editorDestroyed (Steinberg::Vst::EditorView* editor) SMTG_OVERRIDE;

    // from IDataExchangeReceiver
    void PLUGIN_API queueOpened(Steinberg::Vst::DataExchangeUserContextID userContextID, uint32_t blockSize, Steinberg::TBool& dispatchOnBackgroundThread) SMTG_OVERRIDE;
    void PLUGIN_API queueClosed(Steinberg::Vst::DataExchangeUserContextID userContextID) SMTG_OVERRIDE;
    void PLUGIN_API onDataExchangeBlocksReceived(Steinberg::Vst::DataExchangeUserContextID userContextID, uint32_t numBlocks, Steinberg::Vst::DataExchangeBlock* blocks, Steinberg::TBool onBackgroundThread) SMTG_OVERRIDE;




    double getParameter(unsigned int id);
    void setParameter(unsigned int id, double value);
    void sendAction(std::string_view name, const ActionValue& value);
    bool popUiAudioBlock(UiAudioBlock& block);

     //---Interface---------
    DEFINE_INTERFACES
        // Here you can add more supported VST3 interfaces
        // DEF_INTERFACE (Vst::IXXX)
        DEF_INTERFACE (Steinberg::Vst::IDataExchangeReceiver)
    END_DEFINE_INTERFACES (EditController)
    DELEGATE_REFCOUNT (EditController)

//------------------------------------------------------------------------
private:
    SingularityVST3Editor* editor_{nullptr};
    Steinberg::Vst::DataExchangeReceiverHandler dataExchange{this};
    SingularityQueue<UiAudioBlock, 64> queue_;
};

//------------------------------------------------------------------------
