# Singularity

Singularity is an experimental C++ framework for building audio plug-ins with JavaScript-driven user interfaces. Audio processing remains in native C++ while UI components run in [QuickJS](https://github.com/quickjs-ng/quickjs) and render through Skia using a Canvas-like API.

The framework currently produces:

- VST3 plug-ins
- Standalone desktop applications
- Headless Qualcomm AudioReach CAPI modules
- Effect and instrument plug-ins
- Debug builds with JavaScript hot reload
- Release builds with JavaScript and data embedded into the binary

The repository includes an effect and an instrument example. The project is under active development, so APIs and platform behavior may change.

## Architecture

```text
C++ DSP / parameters
        |
SingularityController
        |
QuickJS UI components -> Canvas-like API -> Skia renderer
        |
Standalone host or VST3 host window
```

Important directories:

- `examples/` — example effect and instrument projects
- `widgets/` — reusable JavaScript UI components
- `cmake/` — shared dependency setup and `singularity_create_plugin` orchestration
- `platform/` — native Linux, macOS, and Windows windows
- `standalone/` — standalone target creation plus PipeWire, Core Audio, and ASIO backends
- `vst3/` — VST3 SDK setup, target creation, processor, controller, and view integration
- `capi/` — headless Qualcomm AudioReach CAPI adapter, target creation, entry points, and tests

## Requirements

- CMake 3.22 or newer
- A C++23-capable compiler
- Ninja (recommended)
- Network access during the first configure to fetch QuickJS, Skia, CHOC, dmon, and the VST3 SDK

Linux also requires X11, libportal, PipeWire, FreeType, and Fontconfig development packages. On Ubuntu/Debian:

```sh
sudo apt install ninja-build libx11-dev libxrandr-dev libportal-dev \
  libpipewire-0.3-dev libfreetype6-dev libfontconfig1-dev
```

Windows builds use ASIO and the static MSVC runtime. The current macOS configuration targets arm64.

## Build the Examples

Configure and build a Debug version:

```sh
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug \
  -DSINGULARITY_BUILD_EXAMPLES=ON
cmake --build build --parallel
```

Build a specific application or VST3 target:

```sh
cmake --build build --target ExampleEffect_APP
cmake --build build --target ExampleEffect_VST3
```

For an embedded Release build:

```sh
cmake -S . -B build-release -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DSINGULARITY_BUILD_EXAMPLES=ON
cmake --build build-release --parallel
```

Build products are placed below each example's `out/APP` and `out/VST3` directories in the build tree. Debug builds load `App.js` and widgets from disk so changes can hot reload. Release builds compile the UI with `qjsc` and embed it.

## Create a Plug-in

A plug-in consists of a C++ class implementing the appropriate processing contract, an `App.js` UI, and a CMake target.

```cmake
singularity_create_plugin(MyEffect
    PLUGIN_NAME "My Effect"
    FORMATS APP VST3
    PLUGIN_CLASS MyEffect
    PLUGIN_CLASS_HEADER "MyEffect.h"
    RESOURCES resources/logo.png
)

singularity_configure_vst3(MyEffect
    SUBCATEGORIES Dynamics
)
```

`singularity_configure_vst3` keeps VST3-only metadata out of the shared plug-in
definition. Its first argument identifies the plug-in target, and `SUBCATEGORIES`
are appended to the automatic `Fx` or `Instrument` category (`Fx|Dynamics`
above). Multiple subcategories can be provided as a CMake list. Omit the
configuration to use only the automatic category.

An effect class declares parameters and provides `prepare` and templated `process` methods:

```cpp
class MyEffect {
public:
    static constexpr bool isInstrument = false;
    static constexpr bool isResizable = false;

    static auto getParameters()
    {
        return std::to_array<Parameter>({
            { .id = 1, .name = "Gain", .type = ParamType::Float,
              .minValue = 0.0, .maxValue = 1.0, .defaultValue = 0.5 }
        });
    }

    void prepare(double sampleRate, int maxBlockSize) {}

    template<typename SampleType>
    void process(std::span<const SampleType* const> inputs,
                 std::span<SampleType* const> outputs,
                 int numSamples, ParamList params);
};

static_assert(SingularityPlugin<MyEffect>);
```

### VST3 Preset Files

Singularity exposes component state so a VST3 host can save and restore normal
`.vstpreset` files. Factory preset files should be authored in a host by the
plug-in developer and copied to the standard VST3 factory-preset location by
the product installer. The framework does not generate preset files.

### Built-in Programs and VST3 Program Lists

Use a program collection when the plug-in has persistent, numbered built-in
programs—for example, a multitimbral instrument with a collection per MIDI
part. Ordinary named presets should remain host-managed preset files.

```cpp
static auto getProgramCollections()
{
    return std::to_array<ProgramCollection>({
        {
            .id = "performance-bank",
            .name = "Performance Bank",
            .parameterIds = {13},
            .programs = {
                {
                    .id = "soft",
                    .name = "Soft",
                    .category = "Synth",
                    .parameters = {{13, 0.2}},
                    .data = {std::byte {1}},
                },
            },
        },
    });
}

// VST3-only host layout. This is separate from the reusable program data.
static auto getVst3ProgramUnits()
{
    return std::to_array<Vst3ProgramUnit>({
        {
            .id = 1,
            .parentId = 0,
            .name = "Instrument",
            .eventBusIndex = 0,
            .midiChannel = 0,
        },
    });
}

static auto getVst3ProgramListBindings()
{
    return std::to_array<Vst3ProgramListBinding>({
        {.collectionId = "performance-bank", .unitId = 1},
    });
}

void loadProgramData(
    std::string_view collectionId,
    std::string_view programId,
    std::span<const std::byte> data)
{
    // Apply optional non-parameter program data.
}
```

`ProgramCollection` and `BuiltInProgram` are adapter-neutral. The VST3-only
`Vst3ProgramUnit` and `Vst3ProgramListBinding` types map collections to
`IUnitInfo`, program-selector parameters, and `IProgramListData`. A unit can
own one collection. Collection and program IDs must be non-empty and unique;
collection IDs and program ordering must remain stable because hosts persist
the resulting list ID and program index. Parameters listed in
`ProgramCollection::parameterIds` are assigned to the collection's VST3 unit
without requiring framework parameter groups to mirror VST3 unit IDs.
For instrument plug-ins, `BuiltInProgram::category` is exposed as the VST3
musical-instrument attribute. It is left unset for effect plug-ins.

Program payloads are optional. If any program supplies `data`, the plug-in must
implement `loadProgramData()`. The callback runs when the processor applies a
program, including at a process-block boundary for host program changes, so it
must be safe for the audio-processing thread. Plug-ins without
`getVst3ProgramListBindings()` do not expose `IProgramListData`.

The payload bytes are opaque to Singularity. `getProgramCollections()` can
populate them from compiled resources, generated headers, JSON or another
serialization, compressed data, or references to separately installed
content. Singularity wraps the payload with the program's parameter snapshot
for VST3 transport and returns the payload unchanged to `loadProgramData()`.
Modified program slots are included in VST3 component state so project restore
does not fall back to the original built-in payload. Program lists are fixed
after initialization, and the current VST3 transport limits one program
payload to 64 MiB and all modified payloads in component state to 256 MiB;
large sample libraries should therefore store lightweight identifiers or paths
rather than sample data in each slot.

The UI exports a component from `App.js`:

```js
import { Component } from "singularity";
import { Knob } from "singularity/knob.js";

export default function App() {
  return Component({
    width: 800,
    height: 600,
    backgroundColor: "#181818",
    children: [
      Knob({ x: 40, y: 40, size: 80, parameterId: 1 }),
    ],
  });
}
```

See `examples/ExampleEffect` for a working effect with parameter binding, resources, and an output level meter. `examples/ExampleInstrument` demonstrates the instrument processing signature and MIDI event input.

## Export a Qualcomm CAPI Module

CAPI exports are headless and use the same C++ plugin class as the other
formats. Add `CAPI` to `FORMATS` and point the build at AudioReach Engine CAPI
headers. The CMake target name becomes the entry-point tag registered with AMDB:

```cmake
singularity_create_plugin(MyEffect
    FORMATS CAPI
    PLUGIN_CLASS MyEffect
    PLUGIN_CLASS_HEADER "MyEffect.h"
)
```

Configure a separate CAPI build with the Hexagon toolchain. Supplying the CAPI
SDK directory makes the bundled examples select `FORMATS CAPI`; the framework
then skips the desktop UI and VST3 dependencies automatically:

```sh
cmake -S . -B build-capi -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/hexagon-toolchain.cmake \
  -DSINGULARITY_BUILD_EXAMPLES=ON \
  -DSINGULARITY_CAPI_SDK_DIR=/path/to/audioreach-engine
cmake --build build-capi --target ExampleEffect_CAPI ExampleInstrument_CAPI
```

The CMake target name is also the AMDB entry-point tag. For the target above,
the generated entry points are `MyEffect_get_static_properties` and
`MyEffect_init`. The adapter supports effects as one-input/one-output
modules and instruments as output-buffer-triggered zero-input/one-output source
modules. Effects inherit their output format from `CAPI_INPUT_MEDIA_FORMAT_V2`;
sources must receive `CAPI_OUTPUT_MEDIA_FORMAT_V2`. Audio uses 32-bit
floating-point PCM with `CAPI_DEINTERLEAVED_UNPACKED` buffers. CAPI has no
native MIDI transport, so instrument processing currently receives an empty
MIDI event span. Fixed-point PCM, metadata, and CAPI framework/interface
extensions are not yet bridged. The generic C++ plugin API also does not expose
`CAPI_HEAP_ID`; plugins that allocate dynamically currently use the toolchain's
default C++ allocator and are not island-heap-aware.

CAPI parameter IDs and metadata come directly from `getParameters()`.
`set_param` and `get_param` payloads contain one `float` in the parameter's
plain units. `set_param` changes are queued and applied by the next `process()` call;
read-only values travel back to `get_param` through a separate realtime-safe
queue. Production module and parameter IDs still need to come from the GUID
range allocated by Qualcomm, and AMDB/H2XML registration remains part of the
target AudioReach integration rather than the Singularity binary export.
`getParameters()` is internal C++ metadata; AudioReach clients still need the
parameter IDs and scalar-float payload convention declared in their module API
or generated H2XML configuration.

The host-side adapter tests use the public AudioReach headers and exercise
initialization, effect and source media formats, parameter queues, large-block
chunking, runtime properties, and processing:

```sh
cmake -S . -B build-capi-tests -G Ninja \
  -DSINGULARITY_BUILD_CAPI_TESTS=ON \
  -DSINGULARITY_CAPI_SDK_DIR=/path/to/audioreach-engine
cmake --build build-capi-tests --target SingularityCapiTests
ctest --test-dir build-capi-tests --output-on-failure
```

`ExampleEffect_CAPI` and `ExampleInstrument_CAPI` reuse the same DSP classes as
their APP and VST3 targets; their CAPI binaries do not include either UI.

## Development Status

GitHub Actions checks Release builds on Linux, macOS, and Windows. There is currently no automated unit-test suite or stable public release API. When changing UI loading or resources, validate both Debug hot reload and Release embedding.

Contributions are welcome; see [AGENTS.md](AGENTS.md) for repository conventions.
