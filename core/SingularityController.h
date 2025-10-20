#pragma once

#include "gui/singularity_Webview.h"
#include <memory>

class SingularityController
{
  public:
    SingularityController();
    SingularityController(bool createWindow); // Constructor with window creation control
    virtual ~SingularityController();

    virtual void Initialize() = 0;

    // Public access to webview for VST3 integration
    ISingularityGUI *getWebView() const
    {
        return webview.get();
    }

  protected:
    // virtual void initializeSkiaSurface() = 0;

    int width;
    int height;
    std::unique_ptr<ISingularityGUI> webview;
};

std::unique_ptr<SingularityController> createControllerInstance();
