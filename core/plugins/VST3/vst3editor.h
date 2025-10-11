#pragma once

#include "../../gui/singularity_Webview.h"
#include "public.sdk/source/vst/vsteditcontroller.h"
#include <memory>

using namespace Steinberg;
using namespace Steinberg::Vst;

namespace MyCompanyName
{

//------------------------------------------------------------------------
// SingularityVST3Editor - VST3-specific editor wrapper
//------------------------------------------------------------------------
class SingularityVST3Editor : public EditorView
{
  public:
    SingularityVST3Editor(EditController *controller, const std::string &url = "http://localhost:5173/");
    virtual ~SingularityVST3Editor();

    // EditorView overrides
    tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override;
    tresult PLUGIN_API attached(void *parent, FIDString type) override;
    tresult PLUGIN_API removed() override;
    tresult PLUGIN_API getSize(ViewRect *size) override;
    tresult PLUGIN_API onSize(ViewRect *newSize) override;
    tresult PLUGIN_API canResize() override;
    tresult PLUGIN_API checkSizeConstraint(ViewRect *rect) override;

    // WebView management
    void navigate(const std::string &url);

  protected:
    std::unique_ptr<ISingularityGUI> webview;
    std::string initialUrl;
    int defaultWidth;
    int defaultHeight;
};

} // namespace MyCompanyName