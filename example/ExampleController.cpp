#include "ExampleController.h"

void ExampleController::Initialize()
{
    // Here you should specify the URL to navigate to
    navigate("http://localhost:5173/");

    // Here you should register you parameters
}

std::unique_ptr<SingularityController> createControllerInstance()
{
    return std::make_unique<ExampleController>();
}
