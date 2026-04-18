#include "Example.h"

std::unique_ptr<SingularityPlugin> createPlugin()
{
    return std::make_unique<ExamplePlugin>();
}