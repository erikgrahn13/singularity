# Copilot instructions — Singularity (C++/QuickJS/Skia VST3 + standalone)

Purpose: provide Copilot sessions with the minimal repository knowledge needed to help with builds, UI/JS integration, and conventions.

---

## Build / test / lint (quick reference)

Prereqs:
- CMake >= 3.22, a C++ toolchain (Visual Studio on Windows, clang/gcc on macOS/Linux), network access (FetchContent downloads).

Typical builds use the Example project (it calls add_subdirectory on the repo root):

- Configure + build (Linux/macOS):
  - cmake -S Example -B build -DCMAKE_BUILD_TYPE=Release
  - cmake --build build -- -j

- Configure + build (Windows, Visual Studio x64):
  - cmake -S Example -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
  - cmake --build build --config Release

- Build a single target (example VST3 or APP):
  - cmake --build build --config Release --target Example_VST3
  - cmake --build build --config Release --target Example_APP

Notes:
- The project uses CMake FetchContent to obtain QuickJS and Skia (see cmake/). External artifacts are stored under external/ (gitignored). Ensure network access on first configure.
- Release builds run qjsc to embed UI JS into a generated header; Debug builds load JS from disk for hot reload.

Tests & lint:
- No unit-test framework or CI workflows are present; no lint config (.clang-format/.clang-tidy) is included.

---

## High-level architecture (big picture)

- Purpose: a C++ audio plugin framework with JavaScript-driven UI. Targets include a standalone APP and VST3 plugin.

- Core subsystems:
  - Audio host: SingularityController + plugin targets. Audio callback entry is via SingularityPlugin::process.
  - JS UI engine: IJSEngine / QuickJSEngine (uses QuickJS). Exposes a `ctx` object implementing a Canvas-like API and helpers (parameter binding, event listeners, time/frame info).
  - Rendering: SingularityGraphics + SkiaRenderer / VisageRenderer expose GPU primitives and a canvas API that the JS UI calls into.
  - Packaging: CMake modules (cmake/*) provide FetchContent for QuickJS/Skia and helper function singularity_create_plugin which creates the <name>-shared, <name>_APP and <name>_VST3 targets.
  - Hot reload: file watchers (dmon/chocFileWatcher) drive runtime hot-reload in debug builds; release builds embed JS via qjsc.

- Where to look for specifics:
  - JS API: CANVAS_API.md and QuickJSEngine / QuickJSEngineCanvasAPI.h implement the mapping.
  - Example app/plugin setup: Example/CMakeLists.txt and Example/* (JS + C++).
  - CMake modules and dependency pins: cmake/*.cmake (SKIA.cmake, QUICKJS.cmake, SingularityPlugin.cmake).

---

## Key repository conventions and gotchas

- UI entrypoint & embedding
  - CMake defines UI_MAIN/UI_DIR for a plugin; the UI main file (Example/main.js or hello.js) is the entrypoint.
  - Release builds embed JS using QuickJS's qjsc into build/generated.h (symbol names like qjsc_<stem>). Debug builds intentionally load JS from disk for hot reload.

- JS style / modules
  - QuickJS is used with an ES module loader (js_module_loader). Prefer ES module syntax (import/export) for UI code.
  - widgets/ contains reusable JS components (widgets/*.js). Example/ contains demo UIs (hello.js, basic.js, etc.).

- Parameters & events
  - Audio parameters are exposed via IParameterProvider and bound to JS via js_setParameter / js_getParameter plumbing in QuickJSEngine.
  - UI code should use the provided API to notify the host about parameter changes (see QuickJSEngine methods).

- Build-time expectations
  - external/ holds fetched or prebuilt third-party artifacts and is gitignored. A fresh clone + first cmake configure will populate external/ via FetchContent.
  - On Windows, the project forces the MSVC runtime to MultiThreaded (/MT) to match the prebuilt Skia libraries. Use an x64 generator and a compatible VS toolchain.
  - SKIA and other prebuilt URLs are selected per-platform in cmake/SKIA.cmake — do not change these without confirming platform compatibility.

- Targets naming
  - singularity_create_plugin creates targets named:
    - <name>-shared (static library, used by executable/plugin targets)
    - <name>_APP (standalone executable)
    - <name>_VST3 (VST3 plugin target)
  - Use those names with `cmake --build --target` when you need to build a single artifact.

- Hot reload & development loop
  - Edit JS in widgets/ or Example/; in debug builds the file watcher triggers hot reload. For release testing, run a Release build so the JS is embedded and qjsc is invoked.

- Canonical docs
  - See CANVAS_API.md for the implemented canvas API that JS authors should target.

---

If you want this expanded to include (choose any):
- exact Visual Studio commands and expected binary paths per-OS (I can add),
- a short how-to for embedding new widgets into the release build (qjsc workflow),
- or adding a CI workflow skeleton.

