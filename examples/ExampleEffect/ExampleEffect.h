#pragma once

#include "SingularityPlugin.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <utility>
#include <vector>

class ExampleEffect {
public:
    static constexpr bool isInstrument = false;
    static constexpr bool isResizable = true;

    static auto getParameters()
    {
        return std::to_array<Parameter>({
            Parameter::continuous({
                .id = 13,
                .name = "Volume",
                .minValue = 0.0,
                .maxValue = 1.0,
                .defaultValue = 0.5
            })
        });
    }

    void prepare(double sampleRate, int maxBlockSize) {}

    template<typename SampleType>
    void process(std::span<const SampleType* const> inputs,
                 std::span<SampleType* const> outputs,
                 int numSamples, ParameterValues parameterValues)
    {
        double peak = 0.0; // params.getValueAtSample (15, 0);

        for (int s = 0; s < numSamples; ++s)
        {
            const double volume = parameterValues.get (13, s);
            for (int ch = 0; ch < (int)outputs.size(); ++ch)
            {
                outputs[ch][s] = static_cast<SampleType>(inputs[0][s] * volume);
                peak = std::max(peak, std::abs(static_cast<double>(outputs[ch][s])));
            }
        }

        // params.set (15, std::clamp(peak, 0.0, 1.0));

        sendToUI(0, outputs, numSamples);
    }

    ActionResult handleAction(std::string_view name, const ActionValue& value)
    {
        if (name == "loadSample")
        {
            std::filesystem::path path{*std::get_if<std::string>(&value)};
            std::ifstream file(path, std::ios::binary | std::ios::ate);
            if (!file)
                return {};

            std::vector<std::byte> data(static_cast<std::size_t>(file.tellg()));

            file.seekg(0);

            if (!file.read(reinterpret_cast<char*>(data.data()), static_cast<std::streamsize>(data.size())))
                return {};

            return {{"loadedFileName", path.filename().string()}};
        }

        return std::nullopt;
    }
};

static_assert(SingularityPlugin<ExampleEffect>);
