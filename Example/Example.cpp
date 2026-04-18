#include "Example.h"
#include <cstring>

void ExamplePlugin::process(std::span<const float* const> inputs,
                            std::span<float* const> outputs,
                            int numSamples)
{
    const float* mono = inputs.empty() ? nullptr : inputs[0];
    for (int ch = 0; ch < (int)outputs.size(); ++ch) {
        float* dst = outputs[ch];
        if (mono)
            std::memcpy(dst, mono, numSamples * sizeof(float));
        else
            std::memset(dst, 0, numSamples * sizeof(float));
    }
}

std::unique_ptr<SingularityPlugin> createPlugin()
{
    return std::make_unique<ExamplePlugin>();
}