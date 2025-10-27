# Glint3D

Glint3D is a lightweight OpenGL/GLFW renderer focused on deterministic desktop workflows, JSON‑driven automation, and headless rendering. The previous WebAssembly pipeline has been retired for now; references remain in the history if we decide to bring it back.

## Building (Desktop Only)

```powershell
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build builds/desktop/cmake --config Release
```

The executable expects to be launched from the repository root so resources resolve correctly, or set the `GLINT_RESOURCE_ROOT` environment variable to point at a relocated bundle.

### Runtime Assets

All runtime data lives under `resources/`:
- `resources/shaders/` – GLSL programs loaded by the renderer.
- `resources/assets/` – built-in textures, HDR maps, sample models, and icons.

## JSON Ops

Automation flows go through the `JsonOpsExecutor`. See `schemas/json_ops_v1.json` and the examples under `examples/json-ops/` for the command set (load, transform, lighting, rendering, etc.). These files can be applied via the in-app console or the CLI flags exposed in `engine/src/main.cpp`.

## Testing & Headless Rendering

The CLI supports headless rendering, golden-image style comparisons, and batch JSON ops execution. The scripts under `tests/scripts/` contain the reference invocation patterns (e.g., `run_golden_tests.sh`, `run_integration_tests.sh`).

## Future Web Support

All WebAssembly/WebGL pieces were removed to simplify the project. Reintroducing the browser build will require:

1. Restoring the `EMSCRIPTEN` branch in `CMakeLists.txt` (the current file aborts with a fatal error if configured with Emscripten).
2. Reinstating the wasm bindings, React/Tailwind UI, and associated npm workspace.
3. Re-adding the HTML/JS bridge in `engine/src/main.cpp` and enabling the `WEB_USE_HTML_UI` code paths in the UI layer.

Until then the engine is desktop-only.
