# Singularity

Singularity is an experimental C++ framework for building audio plug-ins with JavaScript-driven user interfaces. Audio processing remains in native C++ while UI components run in [QuickJS](https://github.com/quickjs-ng/quickjs) and render through Skia using a Canvas-like API.

The framework currently produces:

- VST3 plug-ins
- Standalone desktop applications
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
- `cmake/` — dependency setup and `singularity_create_plugin`
- `platform/` — native Linux, macOS, and Windows windows
- `standalone/` — PipeWire, Core Audio, and ASIO audio backends
- `vst3/` — VST3 processor, controller, and view integration

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
```

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

## Development Status

GitHub Actions checks Release builds on Linux, macOS, and Windows. There is currently no automated unit-test suite or stable public release API. When changing UI loading or resources, validate both Debug hot reload and Release embedding.

Contributions are welcome; see [AGENTS.md](AGENTS.md) for repository conventions.
