<!-- Machine Summary Block -->
{"file":"docs/external_dependencies.md","purpose":"Explains how Glint3D manages third-party dependencies after the modular refactor.","exports":[],"depends_on":["third_party","CMakeLists.txt"],"notes":["vendored_vs_managed","optional_features"]}
<!-- Human Summary -->
Developer guide describing which dependencies are vendored, which are managed externally, and how optional modules (ray tracing, denoising, future backends) wire into the build.

# External Dependencies Overview

The modular architecture splits dependencies between **vendored headers** in `third_party/vendored/` and **managed drop-ins** under `third_party/managed/`. Everything is consumed through CMake; no code should include legacy `engine/Libraries` paths.

## Vendored Headers (`third_party/vendored/`)

| Package            | Purpose                                  | Notes                        |
|--------------------|-------------------------------------------|------------------------------|
| `glm`              | Vector/matrix math                        | Header-only                  |
| `stb`              | Image loading/writing helpers             | Header-only                  |
| `rapidjson`        | JSON parsing                              | Used by JsonOps + configs    |
| `imgui`            | ImGui core + demo/backends                | Backends pulled from `imgui/backends/` |
| `tinyexr`          | OpenEXR IO                                | Guarded by `ENABLE_EXR`      |
| `miniz`            | ZIP compression utilities                 | Used by texture export paths |

Add new vendored libraries under `third_party/vendored/<name>/` with a README describing origin/version.

## Managed Dependencies (`third_party/managed/`)

These are optional drop-in binaries/headers that complement system package managers or vcpkg.

- `glfw/` – Prebuilt binaries + include tree for GLFW (if system packages are unavailable).
- `assimp/` – Optional importers (only needed when system Assimp not installed).
- `openimagedenoise/` – Optional CPU denoiser for ray tracing module.

The top-level `CMakeLists.txt` first tries to locate packages via `find_package`. If not found, it probes `third_party/managed/` as a fallback.

## Optional Modules & Flags

| CMake Option              | Description                                  | Dependencies                     |
|---------------------------|----------------------------------------------|----------------------------------|
| `GLINT_ENABLE_RAYTRACING` | Enables CPU ray tracing module               | OIDN (optional denoising)        |
| `GLINT_ENABLE_GIZMOS`     | Toggles ImGui editor gizmos                  | ImGui (vendored)                 |
| `ENABLE_EXR`              | Enables TinyEXR-based HDR/EXR IO             | `tinyexr` (vendored)             |
| `ENABLE_KTX2`             | Grants KTX2/Basis texture support            | `libktx` (externally managed)    |

Future backends (Vulkan, WebGPU) will live under `engine/platform/` alongside their managed dependencies.

## Recommended Package Manager Installs

```bash
# vcpkg (Windows/Linux/macOS)
vcpkg install glm spdlog fmt glfw3 openimagedenoise assimp

# apt (Ubuntu/Debian)
sudo apt install libglm-dev libspdlog-dev libfmt-dev libglfw3-dev libassimp-dev

# Homebrew (macOS)
brew install glm spdlog fmt glfw assimp
```

OIDN and Embree can be installed through the same package managers when enabling ray tracing.

## Legacy Notes

- The old `engine/external/` planning directory has been retired. Any roadmap items from that document should move into `docs/roadmap/` or architecture artifacts as follow-up tasks.
- When adding dependencies, update:
  - `third_party/README.md`
  - `docs/doxygen/Doxyfile` (if exposing new public headers)
  - Relevant task module artifacts
