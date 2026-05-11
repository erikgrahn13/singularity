#pragma once

#include "../core/SingularityController.h"

// Plugin window size configuration

class ExampleController : public SingularityController
{

  public:
    ExampleController() = default;
    void Initialize() override;
};
