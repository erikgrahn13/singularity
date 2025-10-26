#pragma once

#include "AudioManager.h"
#include "SingularityController.h"
#include <sstream>

class StandaloneHelper
{
  public:
    static void initializeView(SingularityController *controller, void *window)
    {
        auto view = ISingularityGUI::createView(window);
        Singularity::Internal::setControllerView(controller, std::move(view));
    }

    static void initializeAudio(SingularityController *controller)
    {
    }
};