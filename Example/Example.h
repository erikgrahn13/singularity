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

    template<typename T>
    void process(std::span<const T* const> inputs,
                 std::span<T* const> outputs,
                 int numSamples,
                 const std::map<int, double>& params)
    {
        if (auto it = params.find(13); it != params.end())
            _volume = it->second;
        const T* mono = inputs.empty() ? nullptr : inputs[0];
        for (int ch = 0; ch < (int)outputs.size(); ++ch) {
            T* dst = outputs[ch];
            if (mono)
                for (int s = 0; s < numSamples; ++s)
                    dst[s] = mono[s] * static_cast<T>(_volume);
            else
                std::memset(dst, 0, numSamples * sizeof(T));
        }
    }
private:
    double _volume = 0.5;
};

static_assert(SingularityPlugin<ExamplePlugin>);
