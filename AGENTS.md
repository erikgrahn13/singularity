# Repository Guidelines

## Project Structure & Module Organization

Core C++ interfaces and implementations live at the repository root, including `SingularityController`, `QuickJSEngine`, and `SkiaRenderer`. Platform-specific window code is under `platform/{linux,macos,windows}/`; standalone audio backends are in `standalone/`, and VST3 integration is in `vst3/`. Reusable QuickJS UI components belong in `widgets/`. CMake dependency and plugin helpers live in `cmake/`. Use `examples/ExampleEffect` and `examples/ExampleInstrument` as reference integrations; their `App.js` files are UI entry points, while `resources/` and `data/` hold packaged assets.

## Build, Test, and Development Commands

The first configure needs network access because CMake fetches dependencies such as QuickJS, Skia, CHOC, and the VST3 SDK.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug -DSINGULARITY_BUILD_EXAMPLES=ON
cmake --build build --parallel
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DSINGULARITY_BUILD_EXAMPLES=ON
cmake --build build-release --config Release
```

Debug builds load JavaScript from disk and support hot reload. Release builds invoke `qjsc` and embed the UI. CI builds Release on Linux, macOS, and Windows. To shorten iteration, append `--target ExampleEffect_APP` or `--target ExampleEffect_VST3` to the build command.

## Coding Style & Naming Conventions

C++ targets use C++23. Follow existing style: four-space indentation, braces on their own line for functions, `PascalCase` types, `camelCase` functions and locals, and trailing underscores for private members (`renderer_`). Keep interfaces named with an `I` prefix, such as `IRenderer`. JavaScript uses ES modules, two-space indentation, `PascalCase` components, and `camelCase` properties. CMake commands and target declarations should remain readable and multiline. No repository-wide formatter or linter is configured, so match the surrounding file.

## Testing Guidelines

There is currently no unit-test framework or coverage threshold. Before submitting, configure and build the examples relevant to the change. Exercise both Debug hot reload and a Release build when changing UI loading, widget imports, resources, or embedding. Platform changes should be validated on the affected OS; the GitHub Actions matrix is the final cross-platform build check.

## Commit & Pull Request Guidelines

Recent commits use short, imperative, lowercase subjects such as `fix plugin name` and `clean up`. Keep each commit focused and explain non-obvious platform or build decisions in its body. Pull requests should summarize behavior, list tested OSes and build targets, link related issues, and include screenshots or recordings for visible UI changes. Do not commit generated `build/` or fetched `external/` content.
