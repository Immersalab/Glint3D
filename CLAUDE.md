# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

---

## Common Development Commands

### Building (Desktop)
```bash
# CMake build with Release configuration
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build builds/desktop/cmake -j

# Visual Studio (Windows) - Modern CMake Workflow
# 1. Generate project files: cmake -S . -B builds/desktop/cmake
# 2. Open builds/desktop/cmake/glint.sln in Visual Studio 2022
# 3. Or open builds/desktop/cmake/glint.vcxproj directly
# 4. Build Configuration: Release|x64 or Debug|x64
# 5. Run from repo root so shaders/ and assets/ paths resolve
# 
# Note: Legacy engine/Project1.vcxproj has been removed.
# Always use CMake-generated project files to ensure consistency.
```

### Building (Web/Emscripten)  
```bash
# Complete web build (engine + frontend)
npm run web:build              # Builds WASM engine + React frontend
./tools/build-web.sh          # Direct script execution
./tools/build-web.bat         # Windows batch script

# Manual engine-only build (if needed)
emcmake cmake -S . -B builds/web/emscripten -DCMAKE_BUILD_TYPE=Release
cmake --build builds/web/emscripten -j

# Serve web build
emrun --no_browser --port 8080 builds/web/emscripten/glint.html
```

### Web UI (React/Tailwind)
```bash
cd web
npm install
npm run dev
# Web UI hosts the engine (WASM) and communicates via JSON Ops

# Desktop app via Tauri
npm run tauri:dev    # Development mode
npm run tauri:build  # Production build

# Or from root directory
npm run dev          # Start development server
npm run build:web    # Build production frontend only
```

## Current Web UI Status (v0.3.0)

### Architecture
- **Frontend**: React 18 + TypeScript + Tailwind CSS + Vite
- **Desktop**: Tauri v1.6 wrapper for native desktop experience
- **Engine Integration**: Emscripten WASM module (`glint3d.js/.wasm/.data`)
- **Communication**: JSON Ops v1 bridge via `app_apply_ops_json()` and `app_scene_to_json()`

### Component Structure
```
web/src/
????????? App.tsx                 # Main application layout with grid system
????????? components/
???   ????????? CanvasHost.tsx      # WebGL canvas initialization and engine loading
???   ????????? Inspector.tsx       # Right panel: scene hierarchy, object/light management
???   ????????? Console.tsx         # Bottom panel: command input and logging
???   ????????? CommandPalette.tsx  # Keyboard shortcuts and quick actions
???   ????????? TopBar.tsx          # Theme toggle, share link, and navigation
????????? sdk/
???   ????????? viewer.ts           # Legacy Emscripten bridge (use @glint3d/wasm-bindings)
????????? main.tsx               # React app entry point
```

### Current Functionality ???
- **Scene Management**: Load/import models (.obj, .ply, .glb, .gltf, .fbx, .dae)
- **Lighting System**: Add/select/delete lights, view light properties
- **Object Operations**: Select, duplicate, remove, nudge transform
- **Import/Export**: Model files, JSON scene files, scene export
- **Rendering**: Headless PNG output with configurable dimensions
- **Desktop Integration**: Tauri native file dialogs and file system access
- **Real-time Updates**: Scene state synchronization every 1 second
- **Theme Support**: Dark/light theme toggle with localStorage persistence
- **Share Links**: URL-based scene state encoding for sharing

### Engine Integration Status ???
- **WASM Build**: Successfully building to `builds/web/emscripten/objviewer.*`
- **Asset Pipeline**: Automatic asset preloading via Emscripten
- **API Bridge**: Complete JSON Ops v1 implementation
- **File System**: Emscripten FS mounting for drag-and-drop and file import
- **Memory Management**: Proper WASM module initialization and cleanup

### Build Configuration ???
- **Vite**: Modern dev server on port 5173 with React plugin
- **Tauri**: Desktop app configuration with 1280x800 default window
- **Emscripten**: CMake build system generating web-compatible artifacts
- **Asset Copying**: Engine build artifacts copied to `web/public/engine/`

### Deployment Status
- **Web**: Ready for static hosting (build outputs to `web/dist/`)
- **Desktop**: Tauri bundle ready for cross-platform distribution
- **CI/CD**: Engine builds automatically generate web artifacts
- **Asset Management**: Engine assets (models, textures, shaders) properly bundled

### Known Limitations & Future Work
- **Performance**: Large model handling could benefit from streaming/LOD
- **Networking**: No multi-user collaboration features yet
- **Advanced Rendering**: Post-processing pipeline not exposed in web UI
- **Mobile**: Touch controls and responsive design not optimized
- **Accessibility**: Screen reader and keyboard navigation need improvement

### Environment Setup (Optional)
For easier command-line usage, you can set up `glint` as a global command:

**Windows:**
```bash
# Run setup script (creates wrapper and adds to PATH)
setup_glint_env.bat
```

**Linux/macOS:**
```bash
# Run setup script (creates wrapper and adds to PATH)
source setup_glint_env.sh
```

After setup, you can use `glint` from any directory instead of the full path:
```bash
# Before setup
./builds/desktop/cmake/Release/glint.exe --ops examples/json-ops/sphere_basic.json --render output.png

# After setup  
glint --ops examples/json-ops/sphere_basic.json --render output.png
```

The setup scripts:
- Create a `glint` wrapper that automatically finds Release/Debug executables
- Add the project root to your PATH environment variable
- Ensure proper working directory for asset path resolution
- Work from any directory while maintaining correct asset paths

### Running Examples
```bash
# Headless rendering with JSON Ops
./builds/desktop/cmake/Debug/glint.exe --ops examples/json-ops/directional-light-test.json --render output.png --w 256 --h 256

# Release build
./builds/desktop/cmake/Release/glint.exe --ops examples/json-ops/directional-light-test.json --render output.png --w 256 --h 256

# CLI with denoise (if OIDN available)  
./builds/desktop/cmake/Debug/glint.exe --ops examples/json-ops/studio-turntable.json --render output.png --denoise

# Using global command (after environment setup)
glint --ops examples/json-ops/sphere_basic.json --render output.png --w 512 --h 512
glint --ops examples/json-ops/directional-light-test.json --render output.png --denoise
```

---

## Architecture Overview

### Highlights (v0.3.0)
- Lights: Point, Directional, and Spot supported end-to-end (engine, shaders, UI, JSON ops). Spot has inverse-square attenuation and smooth cone falloff.
- UI: Add/select/delete lights; per-light enable, intensity; type-specific edits (position, direction, cones for spot). Directional and spot indicators are rendered in the viewport.
- JSON Ops: `add_light` supports `type: point|directional|spot` with fields `position`, `direction`, and `inner_deg`/`outer_deg` for spot. Schema is in `schemas/json_ops_v1.json`.
- CI: Golden image comparison on Linux via ImageMagick (SSIM). Includes a workflow_dispatch job to regenerate candidate goldens.
- Shadows: Temporarily disabled in shaders for deterministic CI until a proper shadow map path is implemented. Re-enable later via a `useShadows` uniform.

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
- **Automatic conversion**: PBR factors ??? Phong approximation for CPU raytracer

#### 3. JSON Ops v1 Bridge (`json_ops.cpp`)
- **Cross-platform scripting**: Identical scene manipulation on desktop and web
- **Operations**: load, duplicate, transform, add_light (point|directional|spot), set_camera, set_camera_preset, render
- **Web integration**: Emscripten exports for `app_apply_ops_json()`, `app_scene_to_json()`
- **State sharing**: URL-based state encoding for shareable scenes

#### 4. Dual Rendering Pipeline
- **OpenGL rasterization**: PBR shaders, multiple render modes (point/wire/solid)
- **CPU raytracer** (`raytracer.cpp`): BVH acceleration, material conversion, optional denoising (OIDN)
- **Offscreen rendering** (`render_offscreen.cpp`): Headless PNG output for automation

**?????? CRITICAL: Dual rendering architecture requires explicit mode selection for advanced materials**

#### 5. UI Architecture
- **Desktop UI**: ImGui panels integrated directly into Application
- **Web UI**: React/Tailwind app (`web/`) communicating via JSON Ops bridge  
- **UI abstraction**: `app_state.h` provides read-only state snapshot for UI layers
- **Command pattern**: `app_commands.h` for UI ??? Application actions

## Dual Rendering Architecture (IMPORTANT)

Glint3D implements **two completely separate rendering pipelines** with different material capabilities. Understanding this is crucial for proper material setup and debugging.

### Rendering Pipeline Comparison

| Feature | **Rasterized** (Default) | **Raytraced** (--raytrace) |
|---------|-------------------------|----------------------------|
| **Performance** | Real-time | Offline (30+ seconds) |
| **Transparency** | Alpha blending only | ??? Full refraction + TIR |
| **Materials** | PBR + legacy Phong | Legacy Material struct only |
| **IOR Usage** | F0 calculation only | ??? Full Snell's law physics |
| **Transmission** | ??? **NOT SUPPORTED** | ??? **FULL SUPPORT** |
| **Fresnel Effects** | Basic at normal incidence | ??? Angle-dependent mixing |
| **Total Internal Reflection** | ??? Not available | ??? Automatic at critical angle |
| **Shader System** | `pbr.frag`, `standard.frag` | CPU-based raytracing |

### Material Property Flow

```cpp
JSON Ops: { "ior": 1.5, "transmission": 0.9 }
           ???
SceneObject dual storage:
????????? material.ior = 1.5          ??? Used by RAYTRACER (Snell's law)
????????? material.transmission = 0.9  ??? Used by RAYTRACER (opacity)  
????????? ior = 1.5                   ??? Used by RASTERIZER (F0 only)
????????? baseColorFactor = color     ??? Used by RASTERIZER (albedo)
```

### Shader Selection Logic (Rasterized Pipeline)

```cpp
// In RenderSystem::renderObjectsBatched()
bool usePBR = (obj.baseColorTex || obj.mrTex || obj.normalTex);
Shader* shader = usePBR && m_pbrShader ? m_pbrShader.get() : m_basicShader.get();
```

- **PBR Shader** (`pbr.vert/frag`): Has `uniform float ior` but **NO** `uniform float transmission`
- **Basic Shader** (`standard.vert/frag`): Legacy Blinn-Phong only

### Critical Usage Notes

#### ??? Common Mistake - Missing --raytrace Flag
```bash
# WRONG: Uses rasterizer (no refraction, looks like regular sphere)
./glint.exe --ops glass-scene.json --render output.png

# Output: Opaque sphere with basic lighting, no glass effects
```

#### ??? Correct Usage for Glass Materials
```bash
# CORRECT: Uses raytracer (full refraction with Fresnel and TIR)
./glint.exe --ops glass-scene.json --render output.png --raytrace

# Output: Realistic glass with:
# - Refraction based on IOR (Snell's law)
# - Fresnel reflection/transmission mixing
# - Total internal reflection at grazing angles
# - Proper caustics and light bending
```

### Material System Architecture

#### SceneObject Material Storage
```cpp
struct SceneObject {
    // Legacy material (used by RAYTRACER)
    Material material;
    ????????? float ior;              // 1.0-3.0 (air=1.0, glass=1.5, diamond=2.42)
    ????????? float transmission;     // 0.0-1.0 (0=opaque, 1=fully transparent) 
    ????????? glm::vec3 diffuse;      // Base color for lighting
    ????????? float roughness;        // Surface roughness for reflections
    ????????? float metallic;         // Metallic vs dielectric
    
    // PBR fields (used by RASTERIZER)  
    glm::vec4 baseColorFactor;  // sRGB color + alpha
    float metallicFactor;       // 0.0-1.0
    float roughnessFactor;      // 0.0-1.0
    float ior;                  // For F0=(n1-n2)??/(n1+n2)?? calculation only
};
```

### Refraction Implementation Details

#### Raytracer Features (refraction.cpp)
- **Snell's Law**: `n???sin(?????) = n???sin(?????)` for accurate ray bending
- **Total Internal Reflection**: Automatic detection when `sin(?????) > 1.0`
- **Fresnel Equations**: Schlick's approximation for reflection/refraction mixing
- **Media Transition**: Automatic entering/exiting material detection
- **Ray Depth**: Supports multiple bounces for complex glass objects

#### JSON Ops Material Assignment
```json
{
  "op": "set_material",
  "target": "GlassSphere", 
  "material": {
    "color": [0.95, 0.98, 1.0],
    "roughness": 0.0,
    "metallic": 0.0,
    "ior": 1.5,           // Crown glass
    "transmission": 0.9    // 90% transparent
  }
}
```

### Debugging Glass Materials

#### 1. Verify Raytracer Activation
Look for this debug output when using `--raytrace`:
```
[RenderSystem] Screen quad initialized for raytracing
[RenderSystem] Raytracing texture initialized (512x512)
[RenderSystem] Loading N objects into raytracer
[DEBUG] Tracing row 0 of 512...
```

#### 2. Check Material Loading
Add debug output in `raytracer.cpp` if materials aren't loading correctly:
```cpp
std::cout << "[DEBUG] Material: ior=" << mat.ior 
          << ", transmission=" << mat.transmission << std::endl;
```

#### 3. Common Material Values
- **Water**: `ior: 1.33, transmission: 0.85`
- **Crown Glass**: `ior: 1.5, transmission: 0.9`  
- **Flint Glass**: `ior: 1.6, transmission: 0.9`
- **Diamond**: `ior: 2.42, transmission: 0.95`
- **Ice**: `ior: 1.31, transmission: 0.8`

### Architecture Limitations

#### Rasterized Pipeline Constraints
- **No Screen-Space Refraction**: Would require depth buffer reconstruction
- **No Multi-Layer Transparency**: Limited to single alpha blend pass
- **No Caustics**: Requires light transport simulation
- **Basic Fresnel**: Only F0 term, no angle-dependent effects

#### Future Enhancements
- **Hybrid Pipeline**: Auto-enable raytracing for transmission > 0
- **Screen-Space Refraction**: Add to PBR shader for real-time glass
- **Material Validation**: Warn when using transmission without raytracing

---

---

## BGFX Migration Strategy

**Status**: Planned for future release
**Goal**: Replace direct OpenGL calls with BGFX for multi-platform rendering and deterministic automation

### Why BGFX?

BGFX is a cross-platform rendering library that provides:
- **Built-in Hardware Abstraction**: Supports OpenGL, DirectX 11/12, Vulkan, Metal, WebGL without custom RHI layer
- **Deterministic Rendering**: Critical for CLI automation, CI/CD golden image testing, and reproducible outputs
- **Shader Cross-Compilation**: Single shader source compiles to all platforms via shaderc
- **Headless Mode**: First-class support for offscreen rendering and batch processing
- **Minimal Overhead**: Lightweight API designed for performance-critical applications

### Current System Problems
- **Direct OpenGL Coupling**: 200+ direct GL calls in render_system.cpp make porting difficult
- **Dual Material Storage**: PBR + legacy Material structs causing conversion drift
- **Pipeline Fragmentation**: Raster (no refraction) vs Ray (full physics) with manual mode selection
- **Platform Lock-in**: WebGL2 requires separate code paths and shader variants
- **Non-Deterministic Output**: Floating-point precision varies across drivers/platforms

### BGFX Architecture Overview

```cpp
// BGFX provides the hardware abstraction layer
namespace bgfx {
    void init(const Init& init);              // Initialize with platform handles
    void frame();                             // Submit frame and swap buffers
    void setViewRect(ViewId id, ...);        // Configure viewport
    void setState(uint64_t state);            // Depth test, blending, culling
    void submit(ViewId id, ProgramHandle h);  // Submit draw call
    TextureHandle readTexture(...);           // Deterministic framebuffer readback
}

// Glint integration strategy
class BgfxRenderSystem {
    void initBgfx();                          // Hook GLFW/headless platform data
    void compileShadersOffline();             // Generate platform-specific shader bins
    void createResourceHandles();             // Buffers, textures, framebuffers
    void submitDrawCalls();                   // Encoder-based submission
    void readbackFramebuffer();               // PNG export via bgfx::readTexture
};
```

### Deterministic Rendering Pipeline

BGFX enables reproducible outputs critical for automation:

```cpp
// Deterministic workflow
1. Seed RNG for raytracer: srand(manifest.seed)
2. Submit BGFX draw calls in stable order (sort by state/depth)
3. Use bgfx::readTexture for pixel-perfect framebuffer capture
4. Generate checksums: SHA256(pixels) → run manifest
5. Validate: compare checksums across platforms/runs
```

### Update: CLI Animation + UI Workspace (Nov 2025)
- `glint render` now supports animation scripts (per-frame TRS + camera) producing PNG sequences; quick overrides (`--camera-pos/target/up/fov`, `--model-translate/rotate-euler/scale`) apply when the script omits that field. Run manifests include per-frame transforms/camera/checksums and an `animation` block.
- `glint ui` launches against the active workspace (auto-detected or via `--workspace`) and the UI exposes **Open Workspace** and **Save Workspace**; legacy boot paths should be removed in favor of workspace-aware startup.

### Key Integration Points

**Files requiring BGFX integration:**
- `engine/core/application/render_system.cpp` - Replace 200+ OpenGL calls with bgfx API
- `engine/core/scene/scene_manager.h` - Unify dual material storage (PBR + legacy)
- `resources/shaders/*.sc` - Convert GLSL to BGFX shader format (use shaderc compiler)
- `engine/modules/raytracer/raytracer.cpp` - Use unified material representation
- `engine/core/application/json_ops.cpp` - Simplify material property handling

**External Dependencies:**
- `third_party/bgfx` - Core rendering library (vendored or managed)
- `third_party/bx` - BGFX base library (platform utilities, math)
- `third_party/bimg` - Image loading/manipulation for BGFX
- `third_party/shaderc` - Offline shader compilation tool

### Expected Outcomes

**For Users:**
- Consistent rendering across Windows/Linux/macOS/Web
- No visual differences between platforms (deterministic output)
- Faster headless rendering with optimized BGFX backends
- Automatic graphics API selection (Vulkan > DirectX > OpenGL)

**For Developers:**
- Single shader codebase (BGFX .sc format) for all platforms
- No manual OpenGL state management (BGFX handles it)
- Built-in profiling and debugging tools (bgfx stats, renderdoc integration)
- Easier to add advanced features (compute shaders, async readback)

**For Automation:**
- Deterministic checksums across CI environments
- Golden image tests work reliably without driver-specific quirks
- Reproducible render manifests with exact provenance tracking

### Migration Priority

BGFX migration is tracked in `ai/tasks/bgfx_cli_automation_overhaul/`. This replaces the previously planned custom RHI layer, as BGFX already provides comprehensive hardware abstraction with better determinism guarantees for CLI automation workflows.

See the BGFX task checklist for detailed implementation phases and validation gates.
- **volk**: Vulkan function loader (when adding Vulkan backend)
- **VMA**: Vulkan Memory Allocator (painless Vulkan memory management)
- **Dawn** or **wgpu-native**: WebGPU desktop implementation

#### **Optional Acceleration** (Desktop Only)
- **Intel Embree**: 2-10x faster CPU ray tracing vs custom BVH
- **Intel OIDN**: Already integrated - AI-based denoising

#### **Build Strategy**
- **Web Preview**: <2MB WASM, raster+SSR-T only, basic materials
- **Desktop Final**: ~50MB executable, hybrid ray/raster, full features

See `docs/external_dependencies.md` for detailed dependency documentation and integration instructions.

### Directory Structure
```
engine/
????????? src/                    # Core C++ implementation
???   ????????? importers/         # Asset import plugins
???   ????????? ui/                # Desktop UI layer (ImGui)  
????????? include/               # Header files
????????? shaders/               # OpenGL/WebGL shaders
????????? assets/                # Models, textures, examples
????????? Libraries/             # Third-party deps (GLFW, ImGui, GLM, stb)

web/                       # React/Tailwind web interface
????????? src/sdk/viewer.ts      # Emscripten bridge wrapper
????????? public/engine/         # WASM build outputs

builds/
????????? desktop/cmake/         # CMake desktop build
????????? web/emscripten/        # Emscripten web build  
????????? vs/x64/                # Visual Studio build outputs
docs/                      # JSON Ops specification & schema
examples/json-ops/         # Sample JSON Ops files (directional-light-test, spot-light-test)
examples/golden/           # Golden images (directional-light-test.png, spot-light-test.png)
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

### Security Features
#### Asset Root Directory Restriction (`--asset-root`)
- **Purpose**: Prevents path traversal attacks in JSON operations by restricting file access to a specified directory
- **Usage**: `glint --asset-root /path/to/assets --ops operations.json`
- **Behavior**:
  - All file paths in JSON ops (load, render_image, set_background skybox) are validated against the asset root
  - Paths containing `..` or resolving outside the asset root are rejected with clear error messages
  - Relative paths are resolved relative to the asset root directory
  - Absolute paths must be within the asset root directory tree
- **Backward Compatibility**: When `--asset-root` is not specified, all paths are allowed (existing behavior)
- **Security**: Blocks attempts like `../../../etc/passwd`, `..\\..\\system32\\config`, URL-encoded traversals
- **Testing**: Run `bash test_path_security.sh` to verify security controls are working

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

**Organized Test Structure:**
All tests are now organized in a hierarchical structure under `tests/` following industry best practices:

```
tests/
????????? unit/                          # C++ unit tests
???   ????????? camera_preset_test.cpp     # Camera preset calculations
???   ????????? path_security_test.cpp     # Path validation logic
???   ????????? test_path_security_build.cpp # Security build validation
????????? integration/                   # Integration tests (JSON ops)
???   ????????? json_ops/
???   ???   ????????? basic/                 # Load, duplicate, transform tests
???   ???   ????????? lighting/              # Light system tests
???   ???   ????????? camera/                # Camera operation tests
???   ???   ????????? materials/             # Material and tone mapping tests
???   ????????? cli/                       # Command-line interface tests
???   ????????? rendering/                 # End-to-end rendering tests
????????? security/                      # Security vulnerability tests
???   ????????? path_traversal/            # Path traversal attack tests
????????? golden/                        # Visual regression testing
???   ????????? scenes/                    # Test scenes for golden generation
???   ????????? references/                # Reference golden images
???   ????????? tools/                     # SSIM comparison tools
????????? data/                          # Test assets and fixtures
????????? scripts/                       # Test automation scripts
????????? results/                       # Test output and artifacts (gitignored)
```

**Test Execution:**
```bash
# Run all test categories
tests/scripts/run_all_tests.sh

# Run specific test categories  
tests/scripts/run_unit_tests.sh        # C++ unit tests
tests/scripts/run_integration_tests.sh # JSON ops integration tests
tests/scripts/run_security_tests.sh    # Security vulnerability tests
tests/scripts/run_golden_tests.sh      # Visual regression tests
```

**Golden Image Testing:**
- Headless renders compared against references via SSIM
- Primary metric: SSIM >= 0.995 (fallback per-channel ?? <= 2 LSB)  
- Jobs render test scenes to `tests/results/golden/renders/` and compare to `tests/golden/references/`
- On failure: CI uploads diff images, heatmaps, and comparison reports as artifacts
- Regenerate goldens: Use workflow_dispatch `regenerate_goldens=true` for candidate generation
- Tools located in `tests/golden/tools/` (golden_image_compare.py, generate_goldens.py)

**Security Testing:**
- Path traversal protection validation via `tests/security/path_traversal/`  
- Verify --asset-root blocks attack vectors: ../../../etc/passwd, ..\\..\\System32, etc.
- Automated via `tests/scripts/run_security_tests.sh`

**Cross-platform & Performance:**
- Verify JSON Ops produces equivalent results on Desktop and Web
- Performance: Log BVH build and frame times in Perf HUD; regressions block PRs

---

## Modularity Rules
- **Engine Core** must not depend on ImGui, GLFW, React, or Emscripten
- **BGFX Abstraction**: When BGFX migration is complete, isolate all graphics API calls within BGFX layer
- **JSON Ops** is the single source of truth between engine and UIs
- New features tested in headless CLI first, then integrated into UI

---

## Application Branding
- **Logo**: Glint3D diamond icon in `engine/assets/img/Glint3DIcon.png`
- **Windows Icon**: Auto-converted to ICO format and embedded in executable via Windows resources
- **Resource File**: `engine/resources/glint3d.rc` includes icon and version information
- **Icon Regeneration**: Use `tools/create_basic_ico.py` when updating logo

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
- **Shadows**: disabled by default in shaders for deterministic CI until a real shadow-map implementation is added (see TODO). Re-enable via `useShadows` when implemented.

---

## Claude Code Usage Notes

### Safe Edits
- Keep edits surgical; avoid repo-wide renames without explicit request
- Don???t change public APIs unless asked; if changed, update docs/tests
- Never modify binary assets or third-party code
- Prefer adapter-level fixes before touching Engine Core

### Useful Requests
- "Prepare codebase for BGFX migration"
- "Add JSON op and update schema/docs/tests"
- "Write golden test for headless renderer"
- "Decouple ImGui/GLFW dependencies from Engine Core"
- "Implement deterministic shadow maps (depth FBO + PCF) and re-enable shadows gate"

### Workflow
1. Outline change and affected files  
2. Apply minimal diffs  
3. Build Desktop + Web  
4. Run tests (unit + golden). If goldens change intentionally, use the regen workflow to produce candidates and update `tests/golden/references/*.png`.  
5. Update docs/schema/recipes

---

## Graphics API Evolution Strategy

### Current Implementation: OpenGL/WebGL2
- **Desktop**: OpenGL 3.3+ core profile with mature feature set
- **Web**: WebGL 2.0 with automatic shader translation (#version 330 core → #version 300 es)
- **Architecture**: Direct OpenGL calls in render_system.cpp (planned for BGFX migration)

### Planned Migration: BGFX Multi-Backend
- **All Platforms**: BGFX provides automatic backend selection (Vulkan, DirectX, Metal, OpenGL, WebGL)
- **Deterministic Output**: Critical for CLI automation and golden image testing
- **Shader Pipeline**: Single .sc shader source compiles to all platforms via shaderc
- **Timeline**: Tracked in `ai/tasks/bgfx_cli_automation_overhaul/`

### Future Vision: Neural Rendering
- **Gaussian Splatting**: Point-based rendering for photorealistic real-time scenes
- **Neural Radiance Fields (NeRF)**: AI-powered view synthesis and scene representation
- **Hybrid Pipeline**: Integration of traditional rasterization with neural techniques

### Development Guidelines for Graphics Evolution
When implementing new features, consider the upcoming BGFX migration:

1. **Minimize OpenGL Coupling**: Avoid adding more direct GL calls; keep rendering logic high-level
2. **Resource Management**: Design with explicit lifetime patterns suitable for modern APIs
3. **Compute Integration**: Plan shader features to work with compute pipelines (post-BGFX)
4. **Cross-Platform Design**: Ensure compatibility patterns that translate across backends
5. **Determinism First**: Prioritize reproducible output for automation workflows

### Current Architecture Status
- **Rendering**: Direct OpenGL calls concentrated in render_system.cpp (~200 calls)
- **Shader System**: GLSL shaders ready for conversion to BGFX .sc format
- **Engine Core**: Modular structure supports BGFX drop-in replacement
- **Asset Pipeline**: Designed to support future rendering techniques

### Implementation Priority
- Phase 1: Complete current CLI platform features and determinism
- Phase 2: BGFX integration (replace OpenGL, unify materials, deterministic readback)
- Phase 3: Advanced rendering features (screen-space effects, hybrid pipeline)
- Phase 4: Neural rendering technique integration

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
- **Path Security**: Use `--asset-root` flag to prevent directory traversal attacks when processing untrusted JSON ops files
- **File Access Control**: All file operations (load, render_image, set_background) validate paths against the configured asset root
- **Defense in Depth**: Path validation includes normalization, traversal detection, and bounds checking

---

*End of file.*
