#pragma once

#include "SingularityPlugin.h"

class ExampleInstrument {
public:
    static constexpr bool isInstrument = true;
    static constexpr bool isResizable = false;

    static auto getParameters()
    {
        return std::to_array<Parameter>({
            { .id = 13, .name = "Volume", .type = ParamType::Float, .minValue = 0.0, .maxValue = 1.0, .defaultValue = 0.5 }
        });
    }

    void prepare(double sampleRate, int maxBlockSize) {}

    template<typename SampleType>
    void process(std::span<SampleType* const> outputs,
                 int numSamples,
                 std::span<const MidiEvent> events,
                 ParamList params)
    {
        double volume = params.get (13);

        for (int s = 0; s < numSamples; ++s)
            for (int ch = 0; ch < (int)outputs.size(); ++ch)
            {
                
            }
    }
};

static_assert(SingularityPlugin<ExampleInstrument>);
