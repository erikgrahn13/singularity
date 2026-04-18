#include "Example.h"
#include <cstring>

ExamplePlugin::ExamplePlugin()
{
    createParameter(13, "Volume", ParamType::Float, 0.5, 0.0, 1.0);
}

void ExamplePlugin::process(std::span<const float *const> inputs,
                            std::span<float *const> outputs,
                            int numSamples,
                            IParameterChanges& paramChanges)
{
    for (int i = 0; i < paramChanges.getCount(); ++i) {
        auto [id, value] = paramChanges.get(i);
        if (id == 13) _volume = static_cast<float>(value);
    }

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