#pragma once

#include "SingularityPlugin.h"
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
                .isList = true,
            },
            {
                .id = 15,
                .name = "Analyzer",
                .shortName = "Analyzr",
                .type = ParamType::Bool,
                .defaultValue = 1.0,
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

        for (int s = 0; s < numSamples; ++s)
            for (int ch = 0; ch < (int)outputs.size(); ++ch)
                outputs[ch][s] = static_cast<SampleType>(inputs[0][s] * volume);
    }
};

static_assert(SingularityPlugin<ExampleEffect>);
