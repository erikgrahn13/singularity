#pragma once

#include "../SingularityPlugin.h"
#include <memory>
#include <map>

class ExamplePlugin : public SingularityPlugin
{
public:
    void process(std::span<const float* const> inputs,
                 std::span<float* const> outputs,
                 int numSamples,
                 const std::map<int, double>& params) override;
private:
    float _volume = 0.5f;
};
