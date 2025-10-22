#pragma once

#include "SingularityController.h"

class StandaloneHelper
{
  public:
    static void initializeView(SingularityController *controller, void *window)
    {
        auto view = ISingularityGUI::createView(window);
        Singularity::Internal::setControllerView(controller, std::move(view));
    }
};