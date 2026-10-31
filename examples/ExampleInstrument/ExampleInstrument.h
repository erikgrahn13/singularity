#pragma once

#include "SingularityPlugin.h"
// #include "vst3/Vst3ProgramLayout.h"
#include <algorithm>

class ExampleInstrument {
public:
    static constexpr bool isInstrument = true;
    static constexpr bool isResizable = false;

    static auto getParameters()
    {
        return std::to_array<Parameter>({
            Parameter::continuous({
                .id = 13,
                .name = "Volume",
                .minValue = 0.0,
                .maxValue = 1.0,
                .defaultValue = 0.5
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
            .outputs = {
                {.name = "Output", .layout = AudioLayout::stereo}
            }
        };
    }

    // static auto getVst3ProgramUnits()
    // {
    //     return std::to_array<Vst3ProgramUnit>({
    //         {
    //             .id = 1,
    //             .parentId = 0,
    //             .name = "Instrument",
    //             .eventBusIndex = 0,
    //             .midiChannel = 0,
    //         },
    //         {
    //             .id = 2,
    //             .parentId = 1,
    //             .name = "Tone",
    //             .eventBusIndex = 0,
    //             .midiChannel = 1,
    //         },
    //     });
    // }

    // static auto getVst3ProgramListBindings()
    // {
    //     return std::to_array<Vst3ProgramListBinding>({
    //         {.collectionId = "performance-bank", .unitId = 1},
    //         {.collectionId = "tone-bank", .unitId = 2},
    //     });
    // }

    // static auto getProgramCollections()
    // {
    //     return std::to_array<ProgramCollection>({
    //         {
    //             .id = "performance-bank",
    //             .name = "Performance Bank",
    //             .parameterIds = {13},
    //             .programs = {
    //                 {
    //                     .id = "default",
    //                     .name = "Default",
    //                     .category = "Synth",
    //                     .parameters = {{13, 0.5}},
    //                     .data = {std::byte {0}},
    //                 },
    //                 {
    //                     .id = "soft",
    //                     .name = "Soft",
    //                     .category = "Synth",
    //                     .parameters = {{13, 0.2}},
    //                     .data = {std::byte {1}},
    //                 },
    //                 {
    //                     .id = "full",
    //                     .name = "Full",
    //                     .category = "Synth",
    //                     .parameters = {{13, 0.9}},
    //                     .data = {std::byte {2}},
    //                 },
    //             },
    //         },
    //         {
    //             .id = "tone-bank",
    //             .name = "Tone Bank",
    //             .parameterIds = {14},
    //             .programs = {
    //                 {
    //                     .id = "dark",
    //                     .name = "Dark",
    //                     .category = "Synth",
    //                     .parameters = {{14, 0.2}},
    //                     .data = {std::byte {3}},
    //                 },
    //                 {
    //                     .id = "bright",
    //                     .name = "Bright",
    //                     .category = "Synth",
    //                     .parameters = {{14, 0.8}},
    //                     .data = {std::byte {4}},
    //                 },
    //             },
    //         },
    //     });
    // }

    // void loadProgramData(
    //     std::string_view,
    //     std::string_view,
    //     std::span<const std::byte> data)
    // {
    //     programVariant_ =
    //         data.empty() ? 0 : std::to_integer<int>(data.front());
    // }

    void prepare(double sampleRate, int maxBlockSize) {}

    template<typename SampleType>
    void process(AudioOutputBuffer<SampleType> outputs,
                 int numSamples,
                 std::span<const MidiEvent>,
                 ParameterValues parameterValues)
    {
        for (auto* output : outputs)
            std::fill_n(output, numSamples, SampleType{});
    }

    ActionResult handleAction(std::string_view name, const ActionValue& value)
    {
        return std::nullopt;
    }

private:
    int programVariant_ = 0;
};

static_assert(SingularityPlugin<ExampleInstrument>);
