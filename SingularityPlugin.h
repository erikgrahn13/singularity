#pragma once
#include <memory>
#include <span>
#include <map>
#include "IParameterProvider.h"

class SingularityPlugin {
public:
    virtual ~SingularityPlugin() = default;

    // Called on the audio thread every buffer.
    // inputs/outputs: non-interleaved float channels.
    // numSamples: buffer length in samples.
    // params: current normalized value of every parameter, keyed by id.
    virtual void process(std::span<const float* const> inputs,
                         std::span<float* const> outputs,
                         int numSamples,
                         const std::map<int, double>& params) {}
};

void registerParameters();
void createParameter(int id, const char* name, ParamType type,
                     double defaultValue, double minValue, double maxValue);
std::unique_ptr<SingularityPlugin> createPlugin();  