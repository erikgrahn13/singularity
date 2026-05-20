#pragma once

#include "../SingularityPlugin.h"

class ExamplePlugin {
public:
    static auto getParameters()
    {
        return std::to_array<Parameter>({
            { .id = 13, .name = "Volume", .type = ParamType::Float, .minValue = 0.0, .maxValue = 1.0, .defaultValue = 0.5 }
        });
    }

    void prepare(double sampleRate, int maxBlockSize) {}

    template<typename SampleType>
    void process(std::span<const SampleType* const> inputs,
                 std::span<SampleType* const> outputs,
                 int numSamples,
                 ParamList params)
    {
        double volume = params.get (13);

        for (int s = 0; s < numSamples; ++s)
            for (int ch = 0; ch < (int)outputs.size(); ++ch)
                outputs[ch][s] = static_cast<SampleType>(inputs[0][s] * volume);
    }
};

static_assert(SingularityPlugin<ExamplePlugin>);
