#include "Example.h"
#include <cstring>

void registerParameters()
{
    createParameter(13, "Volume", ParamType::Float, 0.5, 0.0, 1.0);
}

void ExamplePlugin::process(std::span<const float *const> inputs,
                            std::span<float *const> outputs,
                            int numSamples,
                            const std::map<int, double>& params)
{
    if (auto it = params.find(13); it != params.end())
        _volume = static_cast<float>(it->second);

    const float* mono = inputs.empty() ? nullptr : inputs[0];
    for (int ch = 0; ch < (int)outputs.size(); ++ch) {
        float* dst = outputs[ch];
        if (mono)
            for (int s = 0; s < numSamples; ++s)
                dst[s] = mono[s] * _volume;
        else
            std::memset(dst, 0, numSamples * sizeof(float));
    }
}

std::unique_ptr<SingularityPlugin> createPlugin()
{
    return std::make_unique<ExamplePlugin>();
}