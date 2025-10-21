#pragma once

#include "../core/SingularityController.h"

// Plugin window size configuration

class ExampleController : public SingularityController
{

  public:
    ExampleController();
    ExampleController(bool createWindow); // Constructor for VST3 mode
    void Initialize() override;
    void onViewReady() override;
};
