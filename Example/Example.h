#pragma once

#include "../SingularityPlugin.h"
#include <span>
#include <map>
#include <cstring>

class ExamplePlugin {
public:
    static void registerParameters()
    {
        createParameter(13, "Volume", ParamType::Float, 0.5, 0.0, 1.0);
    }

    void prepare(double sampleRate, int maxBlockSize)
    {
        _sampleRate = sampleRate;
    }

    template<typename SampleType>
    void process(std::span<const SampleType* const> inputs,
                 std::span<SampleType* const> outputs,
                 int numSamples,
                 const std::map<int, double>& params)
    {
        // Parameters
        if (auto it = params.find(13); it != params.end())
            _volume = it->second;

        // Process audio
        for (int ch = 0; ch < outputs.size(); ++ch) {
                for (int s = 0; s < numSamples; ++s)
                    outputs[ch][s] = inputs[0][s] * _volume;
        }
    }
private:
    double _volume = 0.5;
    double _sampleRate = 48000.0;
};

static_assert(SingularityPlugin<ExamplePlugin>);
