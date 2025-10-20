#include "ExampleController.h"

ExampleController::ExampleController()
{
    // setSize(PLUGIN_WIDTH, PLUGIN_HEIGHT);
    int hej;
    hej = 3;
}

ExampleController::ExampleController(bool createWindow) : SingularityController(createWindow)
{
    // Pass createWindow to base class
    // setSize(PLUGIN_WIDTH, PLUGIN_HEIGHT);
    int hej;
    hej = 3;
}

void ExampleController::Initialize()
{
    // Here you should register you parameters
}

std::unique_ptr<SingularityController> createControllerInstance()
{
    return std::make_unique<ExampleController>();
}

// Factory function for VST3 mode (no window creation)
std::unique_ptr<SingularityController> createControllerInstanceForVST3()
{
    return std::make_unique<ExampleController>(false); // false = don't create window
}