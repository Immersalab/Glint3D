# Architecture Migration Plan: Modular Engine Layout

**Date**: 2025-11-02
**Task**: engine_modular_architecture
**Status**: Planning Phase
**Risk Level**: Medium (incremental approach minimizes disruption)

---

## Executive Summary

This document outlines the step-by-step migration from the current flat directory structure to a modular, maintainable architecture. The migration is designed to be **incremental**, **testable**, and **reversible** at each phase.

### Goals
1. **Modularity**: Clear separation between core engine, optional features, and platform code
2. **Maintainability**: Predictable locations for third-party dependencies
3. **Scalability**: Support future backends (Vulkan, WebGPU) and build configurations
4. **Clean Output**: Separate generated artifacts from source code

### Success Criteria
- ✅ All builds (desktop, web) succeed from clean tree
- ✅ No dead code or temporary shims remaining
- ✅ Documentation reflects new structure
- ✅ CI validates build matrix across platforms

---

## Current State (Baseline)

```
Glint3D/
├── engine/
│   ├── src/                 # 44 .cpp files (flat)
│   ├── include/             # 44 .h files (flat)
│   ├── Libraries/           # 390 vendored files (mixed ownership)
│   └── external/            # Empty (planning doc only)
├── resources/               # Assets, shaders
├── renders/                 # Mixed outputs
└── CMakeLists.txt           # Monolithic build
```

**Issues**:
- Flat `src/` makes it hard to distinguish core vs optional features
- `Libraries/` mixes header-only libs with pre-compiled binaries
- `renders/` conflates examples with user outputs
- ImGui impl files split between `engine/` and `Libraries/`

---

## Target State (Post-Migration)

```
Glint3D/
├── engine/
│   ├── core/                # Core engine systems (~20 files)
│   │   ├── application/
│   │   ├── scene/
│   │   ├── rendering/
│   │   └── io/
│   ├── modules/             # Optional features (~24 files)
│   │   ├── raytracing/      # CPU raytracer (optional build)
│   │   ├── gizmos/          # Editor-only features
│   │   └── post_processing/ # Denoising, effects
│   ├── platform/            # Platform-specific code
│   │   ├── desktop/         # GLFW + ImGui
│   │   └── web/             # Emscripten (future)
│   └── include/             # Public API headers (subset)
├── third_party/
│   ├── vendored/            # Header-only libs (GLM, stb, ImGui)
│   └── managed/             # System deps via vcpkg (GLFW, OIDN)
├── resources/               # Unchanged
├── output/                  # Generated artifacts
│   ├── renders/             # User render outputs
│   ├── exports/             # Scene/model exports
│   └── cache/               # Shader cache, thumbnails
└── CMakeLists.txt           # Modular, target-based build
```

---

## Migration Phases

### Phase 1: Establish Scaffolding (Low Risk)
**Duration**: 1-2 hours
**Rollback**: Simply delete new directories

#### 1.1 Create New Directory Structure
```bash
mkdir -p engine/core/{application,scene,rendering,io}
mkdir -p engine/modules/{raytracing,gizmos,post_processing}
mkdir -p engine/platform/desktop
mkdir -p third_party/{vendored,managed}
mkdir -p output/{renders,exports,cache}
```

#### 1.2 Add Placeholder READMEs
Create `README.md` in each new directory explaining its purpose:

**Example** (`engine/core/README.md`):
```markdown
# Engine Core

Contains essential engine systems required by all builds.

- **application/**: Application lifecycle, windowing abstraction
- **scene/**: Scene graph, objects, materials, lights
- **rendering/**: Shader/texture management, render abstractions
- **io/**: Asset loading, file I/O, path management
```

#### 1.3 Update `.gitignore`
```gitignore
# Generated outputs (preserve structure with .gitkeep)
output/renders/*
!output/renders/.gitkeep
!output/renders/README.md
output/exports/*
!output/exports/.gitkeep
output/cache/*
!output/cache/.gitkeep
```

#### 1.4 Verify Build Still Works
```bash
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build builds/desktop/cmake --config Release
```

**Rollback**: `git clean -fd` to remove new directories

---

### Phase 2: Migrate Output Directory (Low Risk)
**Duration**: 30 minutes
**Rollback**: Move files back to `renders/`

#### 2.1 Create Output Structure
```bash
mkdir -p output/renders
cp renders/*.png output/renders/
cp renders/.gitignore output/renders/
cp renders/.gitkeep output/renders/
```

#### 2.2 Update Documentation
- Update `CLAUDE.md` line 493+ with new output paths
- Update `README.md` build instructions
- Update JSON ops examples if they reference `renders/`

#### 2.3 Update Render Path References
Search for hardcoded paths:
```bash
grep -r "renders/" --include="*.cpp" --include="*.h" --include="*.json"
```

Update found references to use `output/renders/`.

#### 2.4 Remove Old `renders/` Directory
```bash
rm -rf renders/
```

#### 2.5 Verify Example Scripts
Test headless rendering:
```bash
./builds/desktop/cmake/Release/glint.exe --ops examples/json-ops/sphere_basic.json --render output/renders/test.png
```

**Rollback**: `git checkout renders/` and revert path changes

---

### Phase 3: Migrate Third-Party Dependencies (Medium Risk)
**Duration**: 2-3 hours
**Rollback**: Keep temporary CMake bridges

#### 3.1 Move Header-Only Libraries
```bash
# Vendored libs that should stay in-tree
mv engine/Libraries/include/glm third_party/vendored/
mv engine/Libraries/include/stb third_party/vendored/
mv engine/Libraries/include/rapidjson third_party/vendored/
mv engine/Libraries/include/tinyexr.h third_party/vendored/tinyexr/
mv engine/Libraries/include/miniz* third_party/vendored/miniz/
mv engine/Libraries/src/miniz* third_party/vendored/miniz/
```

#### 3.2 Move ImGui (Special Handling)
```bash
# ImGui core + backends
mv engine/Libraries/include/imgui third_party/vendored/imgui/
# Move GLFW/OpenGL3 impl files to platform layer
mv engine/imgui_impl_glfw.cpp engine/platform/desktop/
mv engine/imgui_impl_opengl3.cpp engine/platform/desktop/
# Headers stay in third_party but are referenced from platform
```

#### 3.3 Document External Dependencies
Create `third_party/managed/README.md`:
```markdown
# Managed Dependencies

These libraries should be installed via system package manager or vcpkg:

- **GLFW**: Windowing and input (vcpkg: glfw3)
- **OpenImageDenoise**: AI denoising (vcpkg: openimagedenoise)
- **Assimp**: Multi-format asset loading (vcpkg: assimp)

Install via vcpkg:
\`\`\`bash
vcpkg install glfw3 openimagedenoise assimp
\`\`\`
```

#### 3.4 Update CMake Include Paths (Transitional)
```cmake
# Add compatibility aliases (mark with TODO: REMOVE)
set(LEGACY_INCLUDE_COMPAT ON CACHE BOOL "Enable legacy include path compatibility")

if(LEGACY_INCLUDE_COMPAT)
    # TODO: REMOVE after migration complete
    target_include_directories(glint_core PUBLIC
        engine/Libraries/include  # LEGACY
        third_party/vendored      # NEW
    )
else()
    target_include_directories(glint_core PUBLIC
        third_party/vendored
    )
endif()
```

#### 3.5 Update Source Includes (Incremental)
No changes needed if CMake transitional mode is active. Includes like:
```cpp
#include <glm/glm.hpp>
#include "stb_image.h"
```
...will work with both old and new paths.

#### 3.6 Test Build
```bash
cmake -S . -B builds/desktop/cmake -DLEGACY_INCLUDE_COMPAT=ON
cmake --build builds/desktop/cmake
```

**Rollback**: Keep `LEGACY_INCLUDE_COMPAT=ON` and don't delete `engine/Libraries/`

---

### Phase 4: Migrate Core Engine (High Risk)
**Duration**: 4-6 hours
**Rollback**: Git branch + careful testing

#### 4.1 Create Git Branch
```bash
git checkout -b refactor/modular-engine-core
git add .
git commit -m "Checkpoint: Before core engine migration"
```

#### 4.2 Categorize Source Files

**Core Application** (→ `engine/core/application/`):
- `main.cpp`, `application_core.cpp`, `cli_parser.cpp`
- `render_settings.cpp`, `schema_validator.cpp`
- `camera_controller.cpp`, `input_handler.cpp`

**Core Scene** (→ `engine/core/scene/`):
- `scene_manager.cpp`, `light.cpp`, `material.cpp`
- `pbr_material.cpp`, `json_ops.cpp`

**Core Rendering** (→ `engine/core/rendering/`):
- `render_system.cpp`, `shader.cpp`, `texture.cpp`
- `texture_cache.cpp`, `skybox.cpp`, `ibl_system.cpp`

**Core I/O** (→ `engine/core/io/`):
- `objloader.cpp`, `mesh_loader.cpp`, `importer_registry.cpp`
- `importers/*.cpp`, `assimp_loader.cpp`
- `file_dialog.cpp`, `image_io.cpp`
- `resource_paths.cpp`, `user_paths.cpp`, `path_security.cpp`

#### 4.3 Move Files in Batches
```bash
# Batch 1: Application layer
git mv engine/src/main.cpp engine/core/application/
git mv engine/src/application_core.cpp engine/core/application/
# ... repeat for all application files

# Update CMake after each batch:
set(CORE_APP_SOURCES
    engine/core/application/main.cpp
    engine/core/application/application_core.cpp
    # ...
)
```

#### 4.4 Fix Include Paths
After each batch, update includes:
```cpp
// Old:
#include "application_core.h"

// New:
#include "engine/core/application/application_core.h"
```

**OR** update CMakeLists.txt to preserve flat include structure:
```cmake
target_include_directories(glint_core PUBLIC
    engine/core/application
    engine/core/scene
    engine/core/rendering
    engine/core/io
)
```

#### 4.5 Test After Each Batch
```bash
cmake --build builds/desktop/cmake
./builds/desktop/cmake/Release/glint.exe --help
```

**Rollback**: `git reset --hard` to checkpoint commit

---

### Phase 5: Extract Optional Modules (Medium Risk)
**Duration**: 2-3 hours

#### 5.1 Migrate Ray Tracing Module
```bash
# Ray tracing is a self-contained optional feature
mkdir -p engine/modules/raytracing
git mv engine/src/raytracer.cpp engine/modules/raytracing/
git mv engine/src/BVHNode.cpp engine/modules/raytracing/
git mv engine/src/triangle.cpp engine/modules/raytracing/
git mv engine/src/RayUtils.cpp engine/modules/raytracing/
git mv engine/src/brdf.cpp engine/modules/raytracing/
git mv engine/src/microfacet_sampling.cpp engine/modules/raytracing/
git mv engine/src/raytracer_lighting.cpp engine/modules/raytracing/
git mv engine/src/refraction.cpp engine/modules/raytracing/
```

Update CMake:
```cmake
option(GLINT_ENABLE_RAYTRACING "Build CPU raytracer module" ON)

if(GLINT_ENABLE_RAYTRACING)
    set(RAYTRACING_SOURCES
        engine/modules/raytracing/raytracer.cpp
        # ... list all files
    )
    target_sources(glint_core PRIVATE ${RAYTRACING_SOURCES})
    target_compile_definitions(glint_core PUBLIC GLINT_RAYTRACING_ENABLED=1)
endif()
```

#### 5.2 Migrate Gizmo Module
```bash
mkdir -p engine/modules/gizmos
git mv engine/src/grid.cpp engine/modules/gizmos/
git mv engine/src/axisrenderer.cpp engine/modules/gizmos/
git mv engine/src/gizmo.cpp engine/modules/gizmos/
```

Update CMake:
```cmake
option(GLINT_ENABLE_GIZMOS "Build editor gizmo features" ON)

if(GLINT_ENABLE_GIZMOS)
    set(GIZMO_SOURCES
        engine/modules/gizmos/grid.cpp
        engine/modules/gizmos/axisrenderer.cpp
        engine/modules/gizmos/gizmo.cpp
    )
    target_sources(glint_core PRIVATE ${GIZMO_SOURCES})
    target_compile_definitions(glint_core PUBLIC GLINT_GIZMOS_ENABLED=1)
endif()
```

#### 5.3 Test Minimal Build
```bash
# Build without optional features
cmake -S . -B builds/minimal -DGLINT_ENABLE_RAYTRACING=OFF -DGLINT_ENABLE_GIZMOS=OFF
cmake --build builds/minimal
```

Should produce smaller binary with reduced dependencies.

---

### Phase 6: Platform Layer Isolation (Medium Risk)
**Duration**: 2 hours

#### 6.1 Migrate Desktop Platform Code
```bash
mkdir -p engine/platform/desktop
git mv engine/src/ui_bridge.cpp engine/platform/desktop/
git mv engine/src/imgui_ui_layer.cpp engine/platform/desktop/
git mv engine/platform/desktop/imgui_impl_glfw.cpp engine/platform/desktop/  # Already moved in Phase 3
git mv engine/platform/desktop/imgui_impl_opengl3.cpp engine/platform/desktop/
git mv engine/src/file_dialog.cpp engine/platform/desktop/  # Desktop-specific
git mv engine/src/panels/scene_tree_panel.cpp engine/platform/desktop/panels/
```

#### 6.2 Create Platform Abstraction
Future web builds will have `engine/platform/web/` with:
- `ui_bridge_web.cpp` - Web UI state management
- `resource_loader_web.cpp` - Emscripten file system

CMake platform selection:
```cmake
if(EMSCRIPTEN)
    set(PLATFORM_SOURCES engine/platform/web/*.cpp)
else()
    set(PLATFORM_SOURCES engine/platform/desktop/*.cpp)
endif()
```

---

### Phase 7: Clean Up & Harden (Low Risk)
**Duration**: 1-2 hours

#### 7.1 Remove Legacy Directories
```bash
# Verify nothing references old paths
grep -r "engine/Libraries" CMakeLists.txt || echo "Clean"
grep -r "renders/" --include="*.cpp" --include="*.h" || echo "Clean"

# Remove if clean
rm -rf engine/Libraries/
# (Keep .gitkeep in output/ subdirs)
```

#### 7.2 Remove Temporary CMake Flags
```cmake
# Delete these lines:
# set(LEGACY_INCLUDE_COMPAT ON CACHE BOOL "...")
# if(LEGACY_INCLUDE_COMPAT)
#     target_include_directories(... engine/Libraries/include)
# endif()
```

#### 7.3 Update Public Include Directory
```bash
# Only expose essential headers
mkdir -p engine/include
cp engine/core/application/application_core.h engine/include/
cp engine/core/scene/scene_manager.h engine/include/
# ... (selective public API)
```

CMake:
```cmake
target_include_directories(glint_core PUBLIC
    engine/include           # Public API only
    third_party/vendored     # Third-party headers
)

target_include_directories(glint_core PRIVATE
    engine/core              # Internal headers
    engine/modules
    engine/platform
)
```

#### 7.4 Final Build Matrix Validation
Test all configurations:
```bash
# Desktop release
cmake -S . -B builds/desktop-release -DCMAKE_BUILD_TYPE=Release
cmake --build builds/desktop-release

# Desktop debug
cmake -S . -B builds/desktop-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build builds/desktop-debug

# Minimal build
cmake -S . -B builds/minimal -DGLINT_ENABLE_RAYTRACING=OFF -DGLINT_ENABLE_GIZMOS=OFF
cmake --build builds/minimal

# Verify outputs
./builds/desktop-release/Release/glint.exe --version
./builds/desktop-release/Release/glint.exe --ops examples/json-ops/sphere_basic.json --render output/renders/final-test.png
```

Record results in `artifacts/validation/build_matrix.md`.

---

## Risk Mitigation Strategies

### High-Risk Items

**1. CMake Include Path Changes**
- **Risk**: Broken includes cause build failures
- **Mitigation**:
  - Use transitional `LEGACY_INCLUDE_COMPAT` flag
  - Migrate in small batches (10-15 files at a time)
  - Test build after each batch

**2. ImGui Implementation Split**
- **Risk**: ImGui backends in wrong location
- **Mitigation**:
  - Keep `third_party/vendored/imgui/` for core
  - Platform-specific impl files go to `engine/platform/desktop/`
  - Document ownership clearly in READMEs

**3. Shader Path Resolution**
- **Risk**: `GLINT_RESOURCE_ROOT` breaks after migration
- **Mitigation**:
  - `resources/` directory stays at project root (unchanged)
  - No code changes needed, only verify paths work

### Medium-Risk Items

**1. GLFW Library Search**
- **Risk**: Manual library search paths in CMake break
- **Mitigation**:
  - Document vcpkg as primary installation method
  - Test on clean machine with fresh vcpkg install
  - Provide fallback error messages with installation instructions

**2. Git History Preservation**
- **Risk**: Losing file history with `mv` operations
- **Mitigation**:
  - Use `git mv` instead of shell `mv`
  - Verify history with `git log --follow <file>`

### Recovery Plan

If migration fails mid-phase:

```bash
# 1. Return to last checkpoint
git reset --hard <checkpoint-commit>

# 2. Clean build artifacts
rm -rf builds/

# 3. Rebuild from known-good state
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build builds/desktop/cmake

# 4. Document failure in progress.ndjson
echo '{"ts":"...","event":"migration_failure","phase":"X","details":"..."}' >> progress.ndjson
```

---

## Validation Checklist

After completing all phases, verify:

### Build Validation
- [ ] Desktop release build succeeds from clean tree
- [ ] Desktop debug build succeeds
- [ ] Minimal build (no optional features) succeeds
- [ ] Web build (when re-enabled) succeeds
- [ ] No compiler warnings related to missing includes
- [ ] Binary sizes are reasonable (desktop ~50MB, minimal ~20MB)

### Runtime Validation
- [ ] Application launches without crashes
- [ ] Can load example models (OBJ, glTF, PLY)
- [ ] Raytracing renders correctly
- [ ] Headless rendering produces correct output
- [ ] JSON ops execute successfully
- [ ] ImGui UI displays correctly
- [ ] File dialogs work on all platforms

### Documentation Validation
- [ ] `README.md` build instructions updated
- [ ] `CLAUDE.md` directory structure updated
- [ ] `FOR_DEVELOPERS.md` reflects new layout
- [ ] All `artifacts/documentation/*.md` files created
- [ ] No references to old paths (`engine/Libraries`, `renders/`)

### Repository Health
- [ ] No dead code or commented-out legacy paths
- [ ] `.gitignore` updated for `output/` directory
- [ ] No temporary `// TODO: REMOVE` comments remaining
- [ ] Git history preserved (verify with `git log --follow`)
- [ ] All files have Machine Summary Blocks (per FOR_MACHINES.md §0A)

---

## Timeline Estimate

| Phase | Duration | Risk | Dependencies |
|-------|----------|------|--------------|
| 1. Scaffolding | 1-2 hours | Low | None |
| 2. Output Dir | 30 min | Low | Phase 1 |
| 3. Third-Party | 2-3 hours | Medium | Phase 1 |
| 4. Core Engine | 4-6 hours | High | Phase 3 |
| 5. Optional Modules | 2-3 hours | Medium | Phase 4 |
| 6. Platform Layer | 2 hours | Medium | Phase 4 |
| 7. Clean Up | 1-2 hours | Low | Phases 3-6 |
| **Total** | **13-19 hours** | **Medium** | Sequential |

### Execution Strategy
- **Single maintainer**: 2-3 working days
- **Team effort**: 1 day with pair programming
- **Conservative**: Spread over 1 week with testing between phases

---

## Post-Migration Benefits

### Immediate
- ✅ Clear module boundaries (core vs optional)
- ✅ Easier to disable features (smaller binaries)
- ✅ Predictable third-party dependency locations
- ✅ Clean output directory separation

### Long-Term
- ✅ Easier to add new backends (Vulkan, WebGPU)
- ✅ Simplified onboarding for new contributors
- ✅ Better IDE project organization
- ✅ Reduced cognitive load when navigating codebase
- ✅ Clearer ownership and responsibilities

---

## Rollback Strategy

### Full Rollback (Emergency)
```bash
git checkout main
git branch -D refactor/modular-engine-core
# Continue working on old structure
```

### Partial Rollback (Specific Phase)
```bash
# Keep completed phases, rollback current phase
git log --oneline | grep "Checkpoint"
git reset --hard <checkpoint-hash>
# Fix issues and retry phase
```

### Incremental Fix (Preferred)
```bash
# Fix issues without full rollback
git commit -m "Fix: Incorrect include path in shader.cpp"
git push --force-with-lease origin refactor/modular-engine-core
```

---

## Success Metrics

Measure migration success by:

1. **Build Time**: Should be comparable (±10%) to baseline
2. **Binary Size**: Desktop build ~50MB, minimal ~20MB
3. **Test Coverage**: All golden image tests pass
4. **Documentation**: All references to old structure removed
5. **Developer Feedback**: Contributors can navigate new structure easily

---

**Migration Plan Completed**: 2025-11-02
**Next Step**: Create `directory_mapping_matrix.md` for reference during migration
