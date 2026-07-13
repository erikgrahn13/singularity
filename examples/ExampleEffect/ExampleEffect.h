#pragma once

#include "SingularityPlugin.h"
#include <algorithm>
#include <cmath>
// #include "embedded/ExampleEffect_generated.h"

class ExampleEffect {
public:
    static constexpr bool isInstrument = false;
    static constexpr bool isResizable = false;

    static auto getParameters()
    {
        return std::to_array<Parameter>({
            {
                .id = 13,
                .name = "Output Volume",
                .shortName = "Volume",
                .units = "linear",
                .type = ParamType::Float,
                .minValue = 0.0,
                .maxValue = 1.0,
                .defaultValue = 0.5,
            },
            {
                .id = 14,
                .name = "Character",
                .shortName = "Char",
                .type = ParamType::Choice,
                .defaultValue = 0.0,
                .steps = 3,
                .choices = { "Clean", "Warm", "Bright" },
            },
            {
                .id = 15,
                .name = "Output Level",
                .shortName = "Level",
                .type = ParamType::Float,
                .minValue = 0.0,
                .maxValue = 1.0,
                .defaultValue = 0.0,
                .automatable = false,
                .readOnly = true,
            },
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
        // The processor seeds output parameters once per block. Reading the
        // current value preserves the maximum across its 16-sample slices.
        double peak = params.get (15);

        for (int s = 0; s < numSamples; ++s)
            for (int ch = 0; ch < (int)outputs.size(); ++ch)
            {
                outputs[ch][s] = static_cast<SampleType>(inputs[0][s] * volume);
                peak = std::max(peak, std::abs(static_cast<double>(outputs[ch][s])));
            }

        params.set (15, std::clamp(peak, 0.0, 1.0));
    }
};

static_assert(SingularityPlugin<ExampleEffect>);
