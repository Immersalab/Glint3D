# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

---

## Common Development Commands

### Building (Desktop)
```bash
# CMake build with Release configuration
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build builds/desktop/cmake -j

# Visual Studio (Windows)
# Open engine/Project1.vcxproj in Visual Studio 2022
# Build Configuration: Release|x64
# Run from repo root so shaders/ and assets/ paths resolve
```

### Building (Web/Emscripten)  
```bash
# Requires Emscripten SDK active (emsdk activate latest && emsdk_env.bat)
emcmake cmake -S . -B builds/web/emscripten -DCMAKE_BUILD_TYPE=Release
cmake --build builds/web/emscripten -j

# Serve web build
emrun --no_browser --port 8080 builds/web/emscripten/glint.html
```

### Web UI (React/Tailwind)
```bash
cd ui
npm install
npm run dev
# Web UI hosts the engine (WASM) and communicates via JSON Ops
```

### Running Examples
```bash
# Headless rendering with JSON Ops
./builds/vs/x64/Release/glint.exe --ops examples/json-ops/three-point-lighting.json --render output.png --w 1280 --h 720

# CLI with denoise (if OIDN available)  
./builds/vs/x64/Release/glint.exe --ops examples/json-ops/studio-turntable.json --render output.png --denoise
```

---

## Architecture Overview

### Core Application Structure
- **Application class** (`application.h/cpp`): Main application lifecycle, OpenGL context, scene management
- **SceneObject struct**: Represents loaded models with VAO/VBO data, materials, textures, and transform matrices
- **Dual rendering paths**: OpenGL rasterization (desktop/web) + CPU raytracer with BVH acceleration
- **Modular UI**: Desktop uses ImGui, Web uses React/Tailwind communicating via JSON Ops bridge

### Key Systems

#### 1. Asset Loading & Import Pipeline
- **Importer registry** (`importer_registry.h/cpp`): Plugin-based asset loading system
- **Built-in OBJ importer** (`importers/obj_importer.cpp`): Core OBJ format support
- **Assimp plugin** (`importers/assimp_importer.cpp`): glTF, FBX, DAE, PLY support (optional)
- **Texture cache** (`texture_cache.h/cpp`): Shared texture management with KTX2/Basis optional support

#### 2. Material System  
- **PBR materials** (`pbr_material.h`): Modern PBR workflow (baseColor, metallic, roughness)
- **Legacy materials** (`material.h/cpp`): Blinn-Phong for raytracer compatibility
- **Automatic conversion**: PBR factors → Phong approximation for CPU raytracer

#### 3. JSON Ops v1 Bridge (`json_ops_v1.cpp`)
- **Cross-platform scripting**: Identical scene manipulation on desktop and web
- **Operations**: load, duplicate, transform, add_light, set_camera, render
- **Web integration**: Emscripten exports for `app_apply_ops_json()`, `app_scene_to_json()`
- **State sharing**: URL-based state encoding for shareable scenes

#### 4. Rendering Pipeline
- **OpenGL rasterization**: PBR shaders, multiple render modes (point/wire/solid)
- **CPU raytracer** (`raytracer.cpp`): BVH acceleration, material conversion, optional denoising (OIDN)
- **Offscreen rendering** (`render_offscreen.cpp`): Headless PNG output for automation

#### 5. UI Architecture
- **Desktop UI**: ImGui panels integrated directly into Application
- **Web UI**: React/Tailwind app (`ui/`) communicating via JSON Ops bridge  
- **UI abstraction**: `app_state.h` provides read-only state snapshot for UI layers
- **Command pattern**: `app_commands.h` for UI → Application actions

### Directory Structure
```
engine/
├── src/                    # Core C++ implementation
│   ├── importers/         # Asset import plugins
│   └── ui/                # UI layer implementations  
├── include/               # Header files
├── shaders/               # OpenGL/WebGL shaders
├── assets/                # Models, textures, examples
└── Libraries/             # Third-party deps (GLFW, ImGui, GLM, stb)

ui/                        # React/Tailwind web interface
├── src/sdk/viewer.ts      # Emscripten bridge wrapper
└── public/engine/         # WASM build outputs

builds/
├── desktop/cmake/         # CMake desktop build
├── web/emscripten/        # Emscripten web build  
└── vs/x64/                # Visual Studio build outputs
docs/                      # JSON Ops specification & schema
examples/json-ops/         # Sample JSON Ops files
```

### Platform Differences
- **Desktop**: Full feature set, Assimp support, OIDN denoising, compute shaders
- **Web**: WebGL2 only, no compute shaders, HTML UI instead of ImGui overlay
- **Shader compatibility**: Auto-conversion from `#version 330 core` to `#version 300 es`

### Build Configuration Notes
- **Optional dependencies**: Assimp (`USE_ASSIMP=1`), OIDN (`OIDN_ENABLED=1`), KTX2 (`ENABLE_KTX2=ON`)
- **vcpkg integration**: Preferred dependency management for Windows builds
- **Emscripten flags**: Asset preloading, WASM exports, WebGL2 targeting
- **C++17 standard**: Required for filesystem operations and modern STL features

---

## Contribution & PR Guidelines
- Branch naming: `feature/<short-desc>`, `fix/<short-desc>`
- Small, atomic commits; use imperative tense (`add`, `fix`, `refactor`)
- PR checklist:
  - [ ] Compiles on Desktop + Web
  - [ ] Docs updated (JSON Ops schema, README, RELEASE.md if needed)
  - [ ] Tests added/updated
  - [ ] No platform-specific code leaked into Engine Core

---

## Testing & Validation
- **Unit tests**: JSON Ops parsing, scene transforms, importer registry
- **Golden tests**: Headless renders → compare image hashes
- **Cross-platform**: Verify JSON Ops produces equivalent results on Desktop and Web
- **Performance**: Log BVH build and frame times in Perf HUD; regressions block PRs

Directory suggestion:
```
tests/
  unit/
  golden/
  data/        # tiny assets only
```

---

## Modularity Rules
- **Engine Core** must not depend on ImGui, GLFW, React, or Emscripten
- **RHI (Render Hardware Interface)**: abstract OpenGL/WebGL away for future Vulkan/Metal backends
- **JSON Ops** is the single source of truth between engine and UIs
- New features tested in headless CLI first, then integrated into UI

---

## Release & Versioning
- **Engine**: tag semver releases (e.g., `v0.3.0`)
- **JSON Ops**: schema versioned; bump minor for additive ops, major for breaking
- **Web UI**: pinned to engine version in `RELEASE.md`

---

## Footguns & Gotchas
- **WebGL2 limitations**: no geometry/tessellation shaders; limited texture formats
- **sRGB handling**: avoid double-gamma (FB vs texture decode)
- **Normals**: recompute if missing; winding order must match
- **Headless paths**: must run from repo root or use absolute paths
- **ImGui**: keep out of Engine Core; treat as view/controller only

---

## Claude Code Usage Notes

### Safe Edits
- Keep edits surgical; avoid repo-wide renames without explicit request
- Don’t change public APIs unless asked; if changed, update docs/tests
- Never modify binary assets or third-party code
- Prefer adapter-level fixes before touching Engine Core

### Useful Requests
- “Abstract GL calls into RHI”
- “Add JSON op and update schema/docs/tests”
- “Write golden test for headless renderer”
- “Decouple ImGui/GLFW dependencies from Engine Core”

### Workflow
1. Outline change and affected files  
2. Apply minimal diffs  
3. Build Desktop + Web  
4. Run tests (unit + golden)  
5. Update docs/schema/recipes

---

## Performance Budget & Metrics
- HUD counters: draw calls, triangles, materials, textures, VRAM estimate, CPU frame time
- **Budgets (default desktop)**:
  - Draw calls < 5k
  - Triangles < 15M
  - Frame time < 16ms @ 1080p
- Suggestions: instancing, LOD/decimation, merging static geometry

---

## Security
- Never commit API keys or `.env`; provide `.env.example`
- Share links must serialize scene state only (no secrets/paths)
- Validate JSON Ops against schema at boundaries

---

*End of file.*
