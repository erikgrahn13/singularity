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
    void navigate(const std::string &url);

    // Public access to webview for VST3 integration
    ISingularityGUI *getWebView() const
    {
        return m_view.get();
    }

    virtual void onViewReady() = 0;

  private:
    friend class WindowsStandalone;

    void setView(std::unique_ptr<ISingularityGUI> view)
    {
        m_view = std::move(view);
        onViewReady();
    }

    int width;
    int height;
    std::unique_ptr<ISingularityGUI> m_view;
};

std::unique_ptr<SingularityController> createControllerInstance();
