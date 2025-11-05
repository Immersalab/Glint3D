# Current Engine Layout Inventory

**Date**: 2025-11-02
**Purpose**: Document the existing Glint3D directory structure before modularization
**Task**: engine_modular_architecture (Phase 1: Discovery)

---

## Directory Structure Overview

```
Glint3D/
├── engine/
│   ├── src/                    # Application & engine implementation (44 .cpp files)
│   ├── include/                # Public headers (44 .h files)
│   ├── Libraries/              # Third-party vendored code (390 files)
│   └── external/               # Future third-party dependencies (planning doc only)
├── resources/
│   ├── assets/                 # Models, textures, icons
│   ├── shaders/                # GLSL shaders
│   └── windows/                # Windows-specific resources
├── renders/                    # Generated render outputs (2 .png files)
├── schemas/                    # JSON schema definitions
├── docs/                       # Documentation
├── web/                        # React/Tauri web UI
└── CMakeLists.txt              # Build configuration
```

---

## 1. Engine Source Code (`engine/src/` - 44 files)

### Core Application Layer (7 files)
- `main.cpp` - Desktop application entry point
- `application_core.cpp` - Main application lifecycle and windowing
- `ui_bridge.cpp` - Desktop UI state management
- `imgui_ui_layer.cpp` - ImGui desktop UI implementation
- `cli_parser.cpp` - Command-line argument parsing
- `render_settings.cpp` - Render configuration management
- `schema_validator.cpp` - JSON schema validation

### Scene Management (4 files)
- `scene_manager.cpp` - Scene graph and object management
- `light.cpp` - Light source management
- `material.cpp` - Material system (legacy Phong)
- `json_ops.cpp` - JSON operations bridge (cross-platform scripting)

### Rendering System (10 files)
- `render_system.cpp` - Primary OpenGL rendering pipeline
- `shader.cpp` - Shader compilation and management
- `texture.cpp` - Texture loading and caching
- `texture_cache.cpp` - Shared texture management
- `camera_controller.cpp` - Camera movement and view management
- `grid.cpp` - Grid rendering
- `axisrenderer.cpp` - Coordinate axis visualization
- `gizmo.cpp` - 3D transform gizmo
- `skybox.cpp` - Skybox/environment rendering
- `ibl_system.cpp` - Image-based lighting system

### Asset Loading (5 files + 2 importers)
- `objloader.cpp` - Legacy OBJ loader
- `mesh_loader.cpp` - Generic mesh loading interface
- `importer_registry.cpp` - Plugin-based importer system
- `assimp_loader.cpp` - Assimp wrapper for multi-format support
- `image_io.cpp` - Image I/O utilities
- `importers/obj_importer.cpp` - Modern OBJ importer plugin
- `importers/assimp_importer.cpp` - Assimp importer plugin

### Ray Tracing Pipeline (9 files)
- `raytracer.cpp` - CPU ray tracing integrator
- `BVHNode.cpp` - Bounding volume hierarchy acceleration
- `triangle.cpp` - Triangle primitives and intersection
- `RayUtils.cpp` - Ray tracing utilities
- `brdf.cpp` - Bidirectional reflectance distribution functions
- `microfacet_sampling.cpp` - Microfacet BRDF sampling
- `raytracer_lighting.cpp` - Ray tracing lighting calculations
- `refraction.cpp` - Refraction and transmission physics

### Platform & I/O (5 files)
- `file_dialog.cpp` - Native file picker dialogs
- `resource_paths.cpp` - Resource path resolution
- `user_paths.cpp` - Cross-platform user data directories
- `path_security.cpp` - Path traversal protection
- `glad.c` - OpenGL function loader

### UI Panels (1 file)
- `panels/scene_tree_panel.cpp` - Scene hierarchy panel

---

## 2. Engine Headers (`engine/include/` - 44 files)

All headers mirror the source structure with corresponding `.h` files. Notable additions:
- `panels/` subdirectory for UI panel headers
- Platform abstraction: `gl_platform.h`
- Configuration: `config_defaults.h`, `help_text.h`, `colors.h`
- Raytracing-specific: `ray.h`, `RayUtils.h`
- PBR materials: `pbr_material.h` (in addition to legacy `material.h`)

---

## 3. Third-Party Libraries (`engine/Libraries/` - 390 files)

### Current Vendored Dependencies

| Library | Location | Files | Purpose | Status |
|---------|----------|-------|---------|--------|
| **GLM** | `include/glm/` | ~200 | Math library (vectors, matrices) | ✅ Keep vendored |
| **ImGui** | `include/imgui/` | ~40 | Desktop UI framework | ✅ Keep vendored |
| **GLFW** | `include/GLFW/`, `lib/` | ~10 | Windowing and input | ⚠️ Prefer system/vcpkg |
| **glad** | `include/glad/`, `include/KHR/` | ~5 | OpenGL loader | ✅ Keep vendored |
| **stb** | `include/stb/` | ~10 | Image I/O (stb_image, stb_image_write) | ✅ Keep vendored |
| **RapidJSON** | `include/rapidjson/` | ~100 | JSON parsing | ✅ Keep vendored |
| **tinyexr** | `include/tinyexr.h` | 1 | EXR/HDR image format | ✅ Keep vendored |
| **miniz** | `include/miniz*.h`, `src/miniz*.c` | 10 | ZIP compression | ✅ Keep vendored |
| **OpenImageDenoise** | `include/OpenImageDenoise/`, `lib/` | ~5 | AI denoising | ⚠️ Optional, prefer system |

### Size Breakdown
- **Headers**: ~380 files in `include/`
- **Pre-compiled libs**: `lib/` contains GLFW and OIDN binaries
- **Source**: `src/` contains miniz implementation (4 .c files)

### Tight Couplings

**CMake Build System**:
```cmake
# Lines 130-136, 145-151: Hardcoded include paths
target_include_directories(glint PRIVATE
    engine/Libraries/include
    engine/Libraries/include/stb
    engine/Libraries/include/imgui
    engine/Libraries/include/imgui/backends
)

# Lines 172-197: Manual GLFW library search
# Searches vcpkg, environment vars, and engine/Libraries/lib
```

**Direct Includes in Source**:
- All engine files use `#include <glm/glm.hpp>` (expect vendored GLM)
- ImGui integration: `imgui_ui_layer.cpp`, `ui_bridge.cpp` depend on vendored ImGui
- Shader compilation: `shader.cpp` uses glad headers directly

---

## 4. Future Dependencies (`engine/external/`)

Currently **empty except for README.md** which documents planned dependencies:

### Planned Structure (from README.md)
```
engine/external/
├── core/                    # Always-required: fmt, spdlog, cgltf
├── shaders/                 # SPIR-V pipeline: shaderc, spirv-cross
├── backends/                # Vulkan: volk, VMA; WebGPU: Dawn/wgpu-native
├── raytracing/              # Embree, OIDN
└── tools/                   # RenderDoc, validation tools
```

### Migration Strategy (from external/README.md)
- **Phase 1**: Add fmt, spdlog, cgltf for better logging and glTF
- **Phase 2**: SPIR-V shader pipeline (shaderc, SPIRV-Cross)
- **Phase 3**: Optional Embree for faster CPU raytracing
- **Phase 4**: Vulkan/WebGPU backend preparation

---

## 5. Resources (`resources/`)

### Asset Organization
```
resources/
├── assets/
│   ├── icons/              # Application icons (.png, .ico)
│   ├── img/                # UI images
│   ├── models/             # Test models (.obj, .gltf, .ply)
│   └── textures/           # Sample textures
├── shaders/                # GLSL shader source (.vert, .frag, .comp)
└── windows/                # Windows resource templates (.rc.in)
```

### Shader Inventory
All shaders in `resources/shaders/`:
- PBR pipeline: `pbr.vert/frag`
- Standard shading: `standard.vert/frag`
- Post-processing: `rayscreen.vert/frag`
- Utilities: `grid.vert/frag`, `axis.vert/frag`, `outline.vert/frag`
- Raytracing: `raytrace.comp` (compute shader)
- Shadow mapping: `shadow_depth.vert/frag`

**Issue**: Shaders marked as deleted in git status but need migration strategy.

---

## 6. Output Directory (`output/renders/`)

### Current Status
- **Location**: `output/renders/` (under consolidated `output/` root)
- **Contents**: PNG render outputs migrated from legacy `renders/`
- **Git Control**: `.gitkeep` retained; `.gitignore` updated to preserve structure-only files
- **Status**: Legacy `renders/` directory removed; commands/UI default to `output/renders/`

### Notes
1. Example and user-generated renders now share the `output/renders/` sandbox, separate from sources.
2. Complementary directories (`output/exports`, `output/cache`) ready for future pipeline stages.
3. CI workflows and helper scripts now emit artifacts into `output/renders/`.

---

## 7. Build System Integration (CMakeLists.txt)

### Key Path References

**Source Collections** (Lines 27-112):
- `APP_SOURCES` - 42 files for desktop executable
- `CORE_SOURCES` - 40 files for static library (no `main.cpp`, adds `resource_paths.cpp`)
- `IMGUI_SOURCES` - 6 files from `engine/Libraries/include/imgui/`

**Include Directories** (Lines 130-136, 145-152):
- `engine/include` - Public engine headers
- `engine` - Root for ImGui impl files
- `engine/Libraries/include` - All vendored dependencies
- `engine/Libraries/include/stb` - STB image libraries
- `engine/Libraries/include/imgui` - ImGui core
- `engine/Libraries/include/imgui/backends` - ImGui platform bindings

**Library Search Paths** (Lines 172-197):
- `${CMAKE_SOURCE_DIR}/engine/Libraries/lib` - Vendored GLFW/OIDN
- vcpkg paths (x64-windows, x64-windows-static)
- Environment variables (GLFW_ROOT, GLFW_HOME)

### Emscripten Build
**Currently disabled** (Line 124-126):
```cmake
if (EMSCRIPTEN)
    message(FATAL_ERROR "Web builds via Emscripten are temporarily disabled.")
endif()
```

---

## 8. Tight Couplings Analysis

### Critical Dependencies

**1. Vendored Library Paths**
- ❌ **Hardcoded**: `engine/Libraries/include` appears in 4 places in CMakeLists.txt
- ❌ **Hardcoded**: `engine/Libraries/lib` used for manual GLFW search
- ⚠️ **Risk**: Any restructuring breaks include paths for GLM, ImGui, stb, RapidJSON

**2. ImGui Integration**
- ✅ **Tight coupling**: ImGui source lives in `Libraries/` but impl files in `engine/`
- Files: `engine/imgui_impl_glfw.cpp`, `engine/imgui_impl_opengl3.cpp`
- Headers: `engine/include/imgui_ui_layer.h`, `engine/src/imgui_ui_layer.cpp`
- **Issue**: Mixed ownership (vendored + custom impl in same tree)

**3. Shader Path Resolution**
- ✅ **Compile-time**: `GLINT_RESOURCE_ROOT` CMake definition (Line 137, 153)
- ✅ **Runtime**: `resource_paths.cpp` handles shader loading
- ⚠️ **Risk**: Shader paths may need updates if resources/ moves

**4. Asset Paths**
- ✅ **Relative**: Most assets referenced via `resources/assets/`
- ⚠️ **Hardcoded**: Some example scripts may reference absolute paths
- ✅ **User paths**: Recently migrated to cross-platform via `user_paths.cpp`

### Circular Dependencies

**None detected** in current structure. Clean separation between:
- Application layer → Scene management → Rendering
- Raytracer → BVH/Triangle primitives
- Importers → Mesh loader → Scene manager

---

## 9. Modularization Opportunities

### Proposed Module Boundaries

**1. Engine Core** (`engine/core/`)
- Application lifecycle, scene management, camera
- Shader/texture management, rendering abstractions
- Platform abstraction (windowing, input, file dialogs)
- ~20 files

**2. Rendering Modules** (`engine/modules/`)
- `rendering/` - OpenGL rasterization pipeline
- `raytracing/` - CPU raytracer (optional build)
- `post_processing/` - Denoising, tonemapping
- `gizmos/` - Grid, axis, transform gizmo (editor features)
- ~24 files

**3. Platform Layer** (`engine/platform/`)
- `desktop/` - GLFW + ImGui desktop app
- `web/` - Emscripten + React web app (future)
- ~4 files

**4. Third-Party** (`third_party/`)
- `vendored/` - Header-only libs (GLM, stb, RapidJSON, tinyexr, miniz)
- `managed/` - System libs via vcpkg/pkg-config (GLFW, OIDN, Assimp)

**5. Output Directory** (`output/`)
- `renders/` - User render outputs
- `exports/` - Model/scene exports
- `cache/` - Shader cache, thumbnails

---

## 10. Migration Risks

### High Risk
1. **ImGui Integration**: Mixed vendored + custom impl files
   - Mitigation: Keep `third_party/vendored/imgui/` with backends
   - Move `imgui_ui_layer.cpp` to `engine/platform/desktop/`

2. **GLFW Library Search**: Complex manual path resolution
   - Mitigation: Prefer CMake config mode, document vcpkg usage

3. **Shader Deletion**: Git shows shaders as deleted
   - Mitigation: Verify shader status before migration

### Medium Risk
1. **Include Path Updates**: All source files need adjusted includes
   - Mitigation: Incremental migration with temporary CMake bridges

2. **Build Matrix**: Desktop + Web builds must remain functional
   - Mitigation: Test after each phase, document build commands

### Low Risk
1. **Output Directory**: Simple move of `renders/` → `output/renders/`
   - Mitigation: Update .gitignore, add README to preserve structure

---

## 11. File Counts Summary

| Component | Files | Size (approx) |
|-----------|-------|---------------|
| Engine source (`engine/src/`) | 44 .cpp | ~15,000 LOC |
| Engine headers (`engine/include/`) | 44 .h | ~5,000 LOC |
| Third-party vendored | 390 files | ~200,000 LOC |
| Shaders (`resources/shaders/`) | ~20 files | ~3,000 LOC |
| Build scripts | 1 CMakeLists.txt | ~350 LOC |
| **Total** | **~500 files** | **~223,000 LOC** |

---

## 12. Next Steps

Based on this inventory, the modularization plan should prioritize:

1. ✅ **Least risky first**: Migrate `renders/` → `output/` structure
2. ✅ **Clear separation**: Move ray tracing to `engine/modules/raytracing/`
3. ⚠️ **Careful planning**: ImGui integration requires dual ownership strategy
4. ⚠️ **Test extensively**: CMake include paths must support transitional period
5. ✅ **Document clearly**: Provide migration guide for external contributors

---

**Inventory completed**: 2025-11-02
**Next artifact**: `architecture_migration_plan.md`
