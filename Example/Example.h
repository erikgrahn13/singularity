#pragma once

#include "../SingularityPlugin.h"
#include <memory>

class ExamplePlugin : public SingularityPlugin
{
public:
    void process(std::span<const float* const> inputs,
                 std::span<float* const> outputs,
                 int numSamples,
                 IParameterChanges& paramChanges) override;
private:
    float _volume = 0.5f;
};
