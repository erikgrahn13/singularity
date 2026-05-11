#pragma once

#include "gui/singularity_Webview.h"
#include <memory>

class ISingularityGUI;
class SingularityController;
class SingularityVST3Controller;

namespace MyCompanyName
{
class SingularityVST3Editor;
}

namespace Singularity::Internal
{
// Internal API - not for users
void setControllerView(SingularityController *controller, std::unique_ptr<ISingularityGUI> view);
} // namespace Singularity::Internal

class SingularityController
{
  public:
    SingularityController();
    virtual ~SingularityController();

    virtual void Initialize() = 0;
    void navigate(const std::string &url);

  private:
    // Only friend the internal namespace function
    friend void Singularity::Internal::setControllerView(SingularityController *, std::unique_ptr<ISingularityGUI>);
    friend class SingularityVST3Controller;
    friend class MyCompanyName::SingularityVST3Editor;

    ISingularityGUI *getWebView() const
    {
        return m_view.get();
    }

    void setView(std::unique_ptr<ISingularityGUI> view)
    {
        m_view = std::move(view);
    }

    std::unique_ptr<ISingularityGUI> m_view;
};

std::unique_ptr<SingularityController> createControllerInstance();
