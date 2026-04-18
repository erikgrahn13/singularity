#pragma once

#include "../SingularityPlugin.h"
#include <memory>

class ExamplePlugin : public SingularityPlugin
{
public:
    void process(std::span<const float* const> inputs,
                 std::span<float* const> outputs,
                 int numSamples) override;
};
