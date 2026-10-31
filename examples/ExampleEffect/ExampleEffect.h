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
                .minValue = -10.0,
                .maxValue = 10.0,
                .defaultValue = 0.0
            }),
            Parameter::boolean({
                .id = 15,
                .name = "Toggle",
                .defaultValue = false
            }),
            Parameter::stepped({
                .id = 16,
                .name = "Stepped",
                .minValue = 1,
                .maxValue = 8,
                .defaultValue = 5
            }),
            Parameter::choice({
                .id = 17,
                .name = "Choices",
                .choices = {"Choice 1", "Choice 2", "Choice 3"},
                .defaultIndex = 0
            }),
            Parameter::continuous({
                .id = 14,
                .name = "Meter",
                .minValue = 0.0,
                .maxValue = 1.0,
                .defaultValue = 0.0
            }).asOutput()
        });
    }

    static AudioConfiguration getAudioConfiguration()
    {
        return {
            .inputs = {
                {.name = "Input", .layout = AudioLayout::stereo},
            },
            .outputs = {
                {.name = "Output", .layout = AudioLayout::stereo}
            }
        };
    }

    void prepare(double sampleRate, int maxBlockSize) {}

    template<typename SampleType>
    void process(AudioInputBuffer<SampleType> inputs,
                 AudioOutputBuffer<SampleType> outputs,
                 int numSamples, ParameterValues parameterValues)
    {
        double peak = 0.0; // params.getValueAtSample (15, 0);

        for (int s = 0; s < numSamples; ++s)
        {
            const double volume = parameterValues.get (13, s);
            for (int ch = 0; ch < (int)outputs.size(); ++ch)
            {
                outputs[ch][s] = inputs[ch][s] * volume;
                peak = std::max(peak, std::abs(static_cast<double>(outputs[ch][s])));
            }
        }

        parameterValues.set (14, peak);

        sendToUI(outputs.first(1), numSamples);
        // sendToUI(0, outputs, numSamples);
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

    UiSender sendToUI;
};

static_assert(SingularityPlugin<ExampleEffect>);
