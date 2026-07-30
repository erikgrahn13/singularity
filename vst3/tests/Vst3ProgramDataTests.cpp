#include PLUGIN_CLASS_HEADER

#include "BuiltInProgram.h"
#include "Vst3ComponentState.h"
#include "Vst3ParameterSupport.h"
#include "Vst3ProgramData.h"
#include "Vst3ProgramLayout.h"
#include "plugincids.h"
#include "public.sdk/source/common/memorystream.h"
#include "public.sdk/source/vst/vstpresetfile.h"
#include "vst3controller.h"
#include "vst3processor.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

namespace {

using namespace Steinberg;
using namespace Steinberg::SingularityVst3;

int failures = 0;

void expect(bool condition, std::string_view message)
{
    if (condition)
        return;
    ++failures;
    std::cerr << "FAIL: " << message << '\n';
}

bool approximatelyEqual(double first, double second)
{
    return std::abs(first - second) < 1.0e-9;
}

const Parameter* findParameter(
    std::span<const Parameter> parameters,
    unsigned int id)
{
    const auto iterator = std::find_if(
        parameters.begin(),
        parameters.end(),
        [id](const auto& parameter) { return parameter.id == id; });
    return iterator == parameters.end() ? nullptr : &*iterator;
}

double stateParameterValue(
    const ComponentState& state,
    Vst::ParamID id,
    double fallback = -1.0)
{
    const auto value = std::find_if(
        state.parameterValues.begin(),
        state.parameterValues.end(),
        [id](const auto& candidate) { return candidate.first == id; });
    return value == state.parameterValues.end() ? fallback : value->second;
}

void testComponentStateSchema()
{
    const auto originalParameters = std::to_array<Parameter>({
        {.id = 10, .name = "First"},
        {.id = 20, .name = "Second"},
    });
    ComponentState original;
    original.bypass = true;
    original.parameterValues = {{10, 0.25}, {20, 0.75}};

    MemoryStream versionedStream;
    expect(
        writeComponentState(
            &versionedStream, originalParameters, original),
        "could not write versioned component state");

    const auto changedParameters = std::to_array<Parameter>({
        {.id = 30, .name = "Added"},
        {.id = 20, .name = "Second"},
        {.id = 10, .name = "First"},
    });
    versionedStream.seek(0, IBStream::kIBSeekSet, nullptr);
    ComponentState migrated;
    expect(
        readComponentState(
            &versionedStream, changedParameters, migrated),
        "could not read versioned state after a parameter schema change");
    expect(
        migrated.bypass &&
            migrated.parameterValues.size() == 2 &&
            approximatelyEqual(stateParameterValue(migrated, 10), 0.25) &&
            approximatelyEqual(stateParameterValue(migrated, 20), 0.75) &&
            stateParameterValue(migrated, 30) < 0.0,
        "versioned state did not map parameter values by stable ID");

    const auto reducedParameters = std::to_array<Parameter>({
        {.id = 20, .name = "Second"},
    });
    versionedStream.seek(0, IBStream::kIBSeekSet, nullptr);
    ComponentState reduced;
    expect(
        readComponentState(
            &versionedStream, reducedParameters, reduced) &&
            reduced.parameterValues.size() == 1 &&
            approximatelyEqual(stateParameterValue(reduced, 20), 0.75),
        "versioned state did not ignore a removed parameter");

    MemoryStream legacyStream;
    IBStreamer legacyWriter(&legacyStream, kLittleEndian);
    expect(
        legacyWriter.writeInt32(1) &&
            legacyWriter.writeDouble(0.125) &&
            legacyWriter.writeDouble(0.875),
        "could not create legacy positional component state");
    legacyStream.seek(0, IBStream::kIBSeekSet, nullptr);
    ComponentState legacy;
    expect(
        readComponentState(
            &legacyStream, originalParameters, legacy) &&
            legacy.bypass &&
            approximatelyEqual(stateParameterValue(legacy, 10), 0.125) &&
            approximatelyEqual(stateParameterValue(legacy, 20), 0.875),
        "legacy positional component state is no longer readable");

    ComponentState oversizedState = original;
    oversizedState.modifiedPrograms.resize(2);
    for (std::size_t slot = 0;
         slot < oversizedState.modifiedPrograms.size();
         ++slot)
    {
        auto& program = oversizedState.modifiedPrograms[slot];
        program.listId = 1;
        program.programIndex = static_cast<int32>(slot);
        program.data.parameters.reserve(32769);
        for (Vst::ParamID id = 0; id < 32769; ++id)
            program.data.parameters.emplace_back(id, 0.0);
    }
    MemoryStream oversizedStream;
    expect(
        !writeComponentState(
            &oversizedStream, originalParameters, oversizedState),
        "component state accepted excessive cumulative program data");

    ProgramData limitedProgram{
        {
            {100, 0.25},
            {200, 0.75},
        },
        {
            std::byte{0x01},
            std::byte{0x02},
        },
    };
    MemoryStream limitedStream;
    expect(
        writeProgramData(&limitedStream, limitedProgram),
        "limited program data did not serialize");

    ProgramData rejectedProgram;
    limitedStream.seek(0, IBStream::kIBSeekSet, nullptr);
    expect(
        !readProgramData(&limitedStream, rejectedProgram, 1, 2),
        "program data reader ignored its parameter limit");
    limitedStream.seek(0, IBStream::kIBSeekSet, nullptr);
    expect(
        !readProgramData(&limitedStream, rejectedProgram, 2, 1),
        "program data reader ignored its payload limit");
}

template<typename PluginClass>
void testProgramLists(
    VST3Processor<PluginClass>& processor,
    VST3Controller& controller)
{
    if constexpr (!HasVst3ProgramLists<PluginClass>)
    {
        void* interfacePointer = nullptr;
        expect(
            processor.queryInterface(
                getTUID<Vst::IProgramListData>(),
                &interfacePointer) == kNoInterface,
            "plug-in without program banks exposes IProgramListData");
        expect(
            controller.getProgramListCount() == 0,
            "plug-in without program banks exposes a program list");
    }
    else
    {
        const auto banks = PluginClass::getProgramCollections();
        const auto units = []()
        {
            if constexpr (HasVst3ProgramUnits<PluginClass>)
                return PluginClass::getVst3ProgramUnits();
            else
                return std::array<Vst3ProgramUnit, 0> {};
        }();
        const auto bindings = PluginClass::getVst3ProgramListBindings();
        expect(
            controller.getProgramListCount() ==
                static_cast<int32>(banks.size()),
            "controller exposes the wrong program-list count");
        expect(
            controller.getUnitCount() ==
                static_cast<int32>(units.size() + 1),
            "controller exposes the wrong VST3 unit count");

        for (int32 bankIndex = 0;
             bankIndex < static_cast<int32>(banks.size());
             ++bankIndex)
        {
            const auto& bank =
                banks[static_cast<std::size_t>(bankIndex)];
            Vst::ProgramListInfo info {};
            expect(
                controller.getProgramListInfo(bankIndex, info) ==
                    kResultTrue,
                "controller could not describe program list");
            const auto expectedListId =
                static_cast<Vst::ProgramListID>(programListId(bank.id));
            expect(
                info.id == expectedListId,
                "program-list ID is not derived from the stable bank ID");
            expect(
                info.programCount ==
                    static_cast<int32>(bank.programs.size()),
                "program-list count does not match ProgramCollection");
            expect(
                processor.programDataSupported(info.id) == kResultTrue,
                "processor does not support controller program list");

            const auto sourceIndex =
                bank.programs.size() > 1 ? int32 {1} : int32 {0};
            MemoryStream presetStream;
            Vst::PresetFile writer(&presetStream);
            expect(
                writer.storeProgramData(
                    static_cast<Vst::IProgramListData*>(&processor),
                    info.id,
                    sourceIndex),
                "could not store IProgramListData in a VST preset stream");
            expect(
                writer.writeChunkList(),
                "could not finish VST program-data preset");

            presetStream.seek(0, IBStream::kIBSeekSet, nullptr);
            Vst::PresetFile reader(&presetStream);
            expect(
                reader.readChunkList(),
                "could not read VST program-data preset");
            auto restoredListId = info.id;
            expect(
                reader.restoreProgramData(
                    static_cast<Vst::IProgramListData*>(&processor),
                    &restoredListId,
                    0),
                "processor could not restore IProgramListData");
            // VST3 SDK 3.8's IUnitInfo overload converts the successful
            // kResultTrue value (zero) directly to bool and therefore reports
            // false even though setUnitProgramData succeeded. Verify the
            // observable controller state below instead.
            reader.restoreProgramData(
                static_cast<Vst::IUnitInfo*>(&controller),
                info.id,
                0);

            MemoryStream restoredData;
            expect(
                processor.getProgramData(
                    info.id, 0, &restoredData) == kResultTrue,
                "processor could not return restored program data");
            restoredData.seek(0, IBStream::kIBSeekSet, nullptr);
            ProgramData decoded;
            expect(
                readProgramData(&restoredData, decoded),
                "restored program data is not decodable");
            expect(
                decoded.payload ==
                    bank.programs[
                        static_cast<std::size_t>(sourceIndex)].data,
                "opaque program payload did not round-trip");

            const auto parameters = PluginClass::getParameters();
            for (const auto& change :
                 bank.programs[
                     static_cast<std::size_t>(sourceIndex)].parameters)
            {
                const auto* parameter =
                    findParameter(parameters, change.id);
                expect(parameter != nullptr, "test program parameter missing");
                if (!parameter)
                    continue;
                expect(
                    approximatelyEqual(
                        controller.getParamNormalized(parameter->id),
                        plainToNormalized(*parameter, change.value)),
                    "controller did not apply synchronized program data");
            }

            MemoryStream corrupt;
            int32 invalidHeader = 0;
            corrupt.write(
                &invalidHeader,
                sizeof(invalidHeader),
                nullptr);
            corrupt.seek(0, IBStream::kIBSeekSet, nullptr);
            expect(
                processor.setProgramData(info.id, 0, &corrupt) ==
                    kResultFalse,
                "processor accepted corrupt program data");
        }

        for (std::size_t index = 0; index < units.size(); ++index)
        {
            Vst::UnitInfo info {};
            expect(
                controller.getUnitInfo(
                    static_cast<int32>(index + 1), info) == kResultTrue,
                "controller could not describe a VST3 program unit");
            expect(
                info.id == units[index].id &&
                    info.parentUnitId == units[index].parentId,
                "controller exposed the wrong VST3 unit topology");

            if (units[index].midiChannel >= 0)
            {
                Vst::UnitID mappedUnit = Vst::kRootUnitId;
                expect(
                    controller.getUnitByBus(
                        Vst::kEvent,
                        Vst::kInput,
                        units[index].eventBusIndex,
                        units[index].midiChannel,
                        mappedUnit) == kResultTrue &&
                        mappedUnit == units[index].id,
                    "controller exposed the wrong MIDI-to-unit mapping");
            }
        }

        for (const auto& binding : bindings)
        {
            const auto collection = std::find_if(
                banks.begin(),
                banks.end(),
                [&binding](const auto& candidate)
                {
                    return candidate.id == binding.collectionId;
                });
            expect(
                collection != banks.end(),
                "VST3 binding refers to a missing program collection");
        }

        MemoryStream componentState;
        expect(
            processor.getState(&componentState) == kResultOk,
            "processor could not save modified program slots");
        componentState.seek(0, IBStream::kIBSeekSet, nullptr);

        VST3Processor<PluginClass> restoredProcessor;
        expect(
            restoredProcessor.initialize(nullptr) == kResultOk,
            "restored processor initialization failed");
        expect(
            restoredProcessor.setState(&componentState) == kResultOk,
            "restored processor rejected component state");

        for (int32 bankIndex = 0;
             bankIndex < static_cast<int32>(banks.size());
             ++bankIndex)
        {
            Vst::ProgramListInfo info {};
            expect(
                controller.getProgramListInfo(bankIndex, info) == kResultTrue,
                "controller lost program-list information");
            const auto& bank =
                banks[static_cast<std::size_t>(bankIndex)];
            const auto sourceIndex =
                bank.programs.size() > 1 ? std::size_t {1} : std::size_t {0};

            MemoryStream restoredData;
            expect(
                restoredProcessor.getProgramData(
                    info.id, 0, &restoredData) == kResultTrue,
                "component state did not restore modified program data");
            restoredData.seek(0, IBStream::kIBSeekSet, nullptr);
            ProgramData decoded;
            expect(
                readProgramData(&restoredData, decoded) &&
                    decoded.payload == bank.programs[sourceIndex].data,
                "component state lost an opaque program payload");
        }
        expect(
            restoredProcessor.terminate() == kResultOk,
            "restored processor termination failed");

        ComponentState cleanState;
        const auto pluginParameters = PluginClass::getParameters();
        for (const auto& parameter : pluginParameters)
        {
            if (!parameter.readOnly)
                cleanState.parameterValues.emplace_back(
                    parameter.id,
                    plainToNormalized(parameter, parameter.defaultValue));
        }
        for (const auto& bank : banks)
        {
            cleanState.programSelections.push_back({
                static_cast<Vst::ProgramListID>(programListId(bank.id)),
                0,
            });
        }

        MemoryStream cleanStateStream;
        expect(
            writeComponentState(
                &cleanStateStream, pluginParameters, cleanState),
            "could not write clean component state");
        cleanStateStream.seek(0, IBStream::kIBSeekSet, nullptr);
        expect(
            processor.setState(&cleanStateStream) == kResultOk,
            "processor rejected clean component state");
        cleanStateStream.seek(0, IBStream::kIBSeekSet, nullptr);
        expect(
            controller.setComponentState(&cleanStateStream) == kResultOk,
            "controller rejected clean component state");

        for (int32 bankIndex = 0;
             bankIndex < static_cast<int32>(banks.size());
             ++bankIndex)
        {
            Vst::ProgramListInfo info {};
            expect(
                controller.getProgramListInfo(bankIndex, info) == kResultTrue,
                "controller lost a program list during clean restore");
            const auto& bank =
                banks[static_cast<std::size_t>(bankIndex)];

            MemoryStream factoryData;
            expect(
                processor.getProgramData(
                    info.id, 0, &factoryData) == kResultTrue,
                "processor could not return reset factory program data");
            factoryData.seek(0, IBStream::kIBSeekSet, nullptr);
            ProgramData decodedFactory;
            expect(
                readProgramData(&factoryData, decodedFactory) &&
                    decodedFactory.payload == bank.programs.front().data,
                "clean state retained a prior processor program override");

            controller.setParamNormalized(info.id, 1.0);
            controller.setParamNormalized(info.id, 0.0);
            for (const auto& change : bank.programs.front().parameters)
            {
                const auto* parameter =
                    findParameter(pluginParameters, change.id);
                expect(
                    parameter &&
                        approximatelyEqual(
                            controller.getParamNormalized(change.id),
                            plainToNormalized(*parameter, change.value)),
                    "clean state retained a prior controller program override");
            }
        }
    }
}

class LifecyclePlugin
{
public:
    static constexpr bool isInstrument = false;
    static inline int prepareCalls = 0;
    static inline int loadCalls = 0;
    static inline int loadedValue = -1;
    static inline bool loadedBeforePrepare = false;

    static auto getParameters()
    {
        return std::to_array<Parameter>({
            {
                .id = 100,
                .name = "Lifecycle",
                .type = ParamType::Float,
                .minValue = 0.0,
                .maxValue = 1.0,
                .defaultValue = 0.0,
            },
        });
    }

    static auto getProgramCollections()
    {
        return std::to_array<ProgramCollection>({
            {
                .id = "lifecycle",
                .name = "Lifecycle",
                .parameterIds = {100},
                .programs = {
                    {
                        .id = "first",
                        .name = "First",
                        .parameters = {{100, 0.0}},
                        .data = {std::byte {1}},
                    },
                    {
                        .id = "second",
                        .name = "Second",
                        .parameters = {{100, 1.0}},
                        .data = {std::byte {2}},
                    },
                },
            },
        });
    }

    static auto getVst3ProgramListBindings()
    {
        return std::to_array<Vst3ProgramListBinding>({
            {.collectionId = "lifecycle", .unitId = 0},
        });
    }

    void prepare(double, int)
    {
        prepared_ = true;
        ++prepareCalls;
    }

    void loadProgramData(
        std::string_view,
        std::string_view,
        std::span<const std::byte> data)
    {
        loadedBeforePrepare = loadedBeforePrepare || !prepared_;
        loadedValue =
            data.empty() ? -1 : std::to_integer<int>(data.front());
        ++loadCalls;
    }

    template<typename SampleType>
    void process(
        std::span<const SampleType* const>,
        std::span<SampleType* const>,
        int,
        ParamList)
    {
    }

private:
    bool prepared_ = false;
};

static_assert(SingularityPlugin<LifecyclePlugin>);

class InspectableLifecycleProcessor
    : public VST3Processor<LifecyclePlugin>
{
public:
    double processingParameter(Vst::ParamID id) const
    {
        const auto parameter = std::find_if(
            mParams.begin(),
            mParams.end(),
            [id](const auto& candidate)
            {
                return candidate.metadata.id == id;
            });
        return parameter == mParams.end() ? -1.0 : parameter->smoothed;
    }

    bool processingBypass() const
    {
        return mBypassProcessorFloat.isActive();
    }
};

void testProgramLifecycle()
{
    LifecyclePlugin::prepareCalls = 0;
    LifecyclePlugin::loadCalls = 0;
    LifecyclePlugin::loadedValue = -1;
    LifecyclePlugin::loadedBeforePrepare = false;

    InspectableLifecycleProcessor processor;
    expect(
        processor.initialize(nullptr) == kResultOk,
        "lifecycle processor initialization failed");
    expect(
        LifecyclePlugin::loadCalls == 0,
        "program data was loaded before processor preparation");

    ComponentState selectedState;
    selectedState.parameterValues = {{100, 0.25}};
    selectedState.programSelections = {{
        static_cast<Vst::ProgramListID>(programListId("lifecycle")),
        1,
    }};
    MemoryStream stateStream;
    const auto lifecycleParameters = LifecyclePlugin::getParameters();
    expect(
        writeComponentState(
            &stateStream, lifecycleParameters, selectedState),
        "could not create lifecycle component state");
    stateStream.seek(0, IBStream::kIBSeekSet, nullptr);
    expect(
        processor.setState(&stateStream) == kResultOk,
        "lifecycle processor rejected pre-prepare state");
    expect(
        LifecyclePlugin::loadCalls == 0,
        "setState loaded program data before processor preparation");

    Vst::ProcessSetup setup {};
    setup.processMode = Vst::kRealtime;
    setup.symbolicSampleSize = Vst::kSample32;
    setup.maxSamplesPerBlock = 64;
    setup.sampleRate = 48000.0;
    expect(
        processor.setupProcessing(setup) == kResultOk,
        "lifecycle processor setup failed");
    expect(
        LifecyclePlugin::prepareCalls == 1 &&
            LifecyclePlugin::loadCalls == 1 &&
            LifecyclePlugin::loadedValue == 2 &&
            !LifecyclePlugin::loadedBeforePrepare,
        "selected program was not applied after preparation");

    MemoryStream savedAfterSetup;
    expect(
        processor.getState(&savedAfterSetup) == kResultOk,
        "lifecycle processor could not save state after setup");
    savedAfterSetup.seek(0, IBStream::kIBSeekSet, nullptr);
    ComponentState decodedAfterSetup;
    expect(
        readComponentState(
            &savedAfterSetup, lifecycleParameters, decodedAfterSetup) &&
            decodedAfterSetup.parameterValues.size() == 1 &&
            approximatelyEqual(
                decodedAfterSetup.parameterValues.front().second, 0.25),
        "deferred program loading overwrote restored working parameters");

    ComponentState processingRestore;
    processingRestore.bypass = true;
    processingRestore.parameterValues = {{100, 0.75}};
    processingRestore.programSelections = {{
        static_cast<Vst::ProgramListID>(programListId("lifecycle")),
        1,
    }};
    MemoryStream processingState;
    expect(
        writeComponentState(
            &processingState, lifecycleParameters, processingRestore),
        "could not create processing-time component state");
    processingState.seek(0, IBStream::kIBSeekSet, nullptr);
    expect(
        processor.setState(&processingState) == kResultOk,
        "processor rejected state while processing was configured");
    expect(
        approximatelyEqual(processor.processingParameter(100), 0.25) &&
            !processor.processingBypass(),
        "setState mutated processing objects on the UI thread");

    MemoryStream savedBeforeBoundary;
    expect(
        processor.getState(&savedBeforeBoundary) == kResultOk,
        "processor could not publish pending state");
    savedBeforeBoundary.seek(0, IBStream::kIBSeekSet, nullptr);
    ComponentState decodedBeforeBoundary;
    expect(
        readComponentState(
            &savedBeforeBoundary,
            lifecycleParameters,
            decodedBeforeBoundary) &&
            decodedBeforeBoundary.bypass &&
            approximatelyEqual(
                stateParameterValue(decodedBeforeBoundary, 100), 0.75),
        "getState did not expose the pending restored state");

    Vst::ProcessData emptyBlock {};
    expect(
        processor.process(emptyBlock) == kResultOk &&
            approximatelyEqual(processor.processingParameter(100), 0.75) &&
            processor.processingBypass(),
        "pending state was not applied at the next process boundary");

    std::atomic<bool> keepProcessing {true};
    std::thread processingThread(
        [&processor, &keepProcessing]()
        {
            Vst::ProcessData block {};
            while (keepProcessing.load(std::memory_order_acquire))
                processor.process(block);
        });
    for (int iteration = 0; iteration < 100; ++iteration)
    {
        const auto expectedValue =
            iteration % 2 == 0 ? 0.2 : 0.8;
        ComponentState concurrentRestore;
        concurrentRestore.bypass = iteration % 2 != 0;
        concurrentRestore.parameterValues = {{100, expectedValue}};
        concurrentRestore.programSelections = {{
            static_cast<Vst::ProgramListID>(
                programListId("lifecycle")),
            1,
        }};
        MemoryStream concurrentState;
        expect(
            writeComponentState(
                &concurrentState,
                lifecycleParameters,
                concurrentRestore),
            "could not create concurrent component state");
        concurrentState.seek(0, IBStream::kIBSeekSet, nullptr);
        expect(
            processor.setState(&concurrentState) == kResultOk,
            "concurrent setState failed");

        MemoryStream concurrentSaved;
        expect(
            processor.getState(&concurrentSaved) == kResultOk,
            "concurrent getState failed");
        concurrentSaved.seek(0, IBStream::kIBSeekSet, nullptr);
        ComponentState decodedConcurrent;
        expect(
            readComponentState(
                &concurrentSaved,
                lifecycleParameters,
                decodedConcurrent) &&
                decodedConcurrent.bypass == concurrentRestore.bypass &&
                approximatelyEqual(
                    stateParameterValue(decodedConcurrent, 100),
                    expectedValue),
            "concurrent state publication returned a torn or stale state");
    }

    const auto lifecycleListId =
        static_cast<Vst::ProgramListID>(programListId("lifecycle"));
    for (int iteration = 0; iteration < 100; ++iteration)
    {
        const auto expectedValue =
            iteration % 2 == 0 ? 0.3 : 0.7;
        ProgramData replacement;
        replacement.parameters = {{100, expectedValue}};
        replacement.payload = {
            static_cast<std::byte>(iteration % 2 == 0 ? 1 : 2),
        };
        MemoryStream replacementStream;
        expect(
            writeProgramData(&replacementStream, replacement),
            "could not create concurrent program data");
        replacementStream.seek(0, IBStream::kIBSeekSet, nullptr);
        expect(
            processor.setProgramData(
                lifecycleListId, 1, &replacementStream) == kResultTrue,
            "concurrent setProgramData failed");

        MemoryStream savedProgram;
        expect(
            processor.getProgramData(
                lifecycleListId, 1, &savedProgram) == kResultTrue,
            "concurrent getProgramData failed");
        savedProgram.seek(0, IBStream::kIBSeekSet, nullptr);
        ProgramData decodedProgram;
        expect(
            readProgramData(&savedProgram, decodedProgram) &&
                approximatelyEqual(
                    stateParameterValue(
                        ComponentState {
                            .parameterValues = decodedProgram.parameters,
                        },
                        100),
                    expectedValue) &&
                decodedProgram.payload == replacement.payload,
            "concurrent program replacement returned stale data");
    }
    keepProcessing.store(false, std::memory_order_release);
    processingThread.join();

    ComponentState staleLifecycleState;
    staleLifecycleState.bypass = true;
    staleLifecycleState.parameterValues = {{100, 0.9}};
    staleLifecycleState.programSelections = {{
        lifecycleListId,
        1,
    }};
    MemoryStream staleLifecycleStream;
    expect(
        writeComponentState(
            &staleLifecycleStream,
            lifecycleParameters,
            staleLifecycleState),
        "could not create stale lifecycle state");
    staleLifecycleStream.seek(0, IBStream::kIBSeekSet, nullptr);
    expect(
        processor.setState(&staleLifecycleStream) == kResultOk,
        "processor rejected state queued before termination");
    expect(
        processor.terminate() == kResultOk,
        "lifecycle processor termination failed");

    LifecyclePlugin::loadedValue = -1;
    expect(
        processor.initialize(nullptr) == kResultOk,
        "lifecycle processor reinitialization failed");
    expect(
        processor.setupProcessing(setup) == kResultOk,
        "reinitialized lifecycle processor setup failed");
    expect(
        approximatelyEqual(processor.processingParameter(100), 0.0) &&
            !processor.processingBypass() &&
            LifecyclePlugin::loadedValue == 1,
        "queued state survived processor termination");
    expect(
        processor.terminate() == kResultOk,
        "reinitialized lifecycle processor termination failed");
}

} // namespace

int main()
{
    VST3Processor<PLUGIN_CLASS> processor;
    VST3Controller controller;
    expect(
        processor.initialize(nullptr) == kResultOk,
        "processor initialization failed");
    expect(
        controller.initialize(nullptr) == kResultOk,
        "controller initialization failed");

    testProgramLists(processor, controller);
    testComponentStateSchema();
    testProgramLifecycle();

    expect(
        controller.terminate() == kResultOk,
        "controller termination failed");
    expect(
        processor.terminate() == kResultOk,
        "processor termination failed");

    if (failures == 0)
        std::cout << "all VST3 program-data tests passed\n";
    return failures == 0 ? 0 : 1;
}
