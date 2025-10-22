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
    controller->setView(std::move(view));
}
} // namespace Singularity::Internal