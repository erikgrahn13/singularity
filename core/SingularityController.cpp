#include "SingularityController.h"

SingularityController::SingularityController()
{
    // Default constructor creates window (for standalone mode)
}

SingularityController::~SingularityController()
{
}

void SingularityController::navigate(const std::string &url)
{
    m_view->navigate(url);
}

namespace Singularity::Internal
{
void setControllerView(SingularityController *controller, std::unique_ptr<ISingularityGUI> view)
{
    // Set the view in the controller FIRST
    controller->setView(std::move(view));
    // THEN initialize the platform webview, which will trigger OnWebViewReady() → callback → Initialize()
    // This ensures m_view is set before Initialize() is called
    controller->m_view->initialize();
}
} // namespace Singularity::Internal