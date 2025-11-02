# Directory Mapping Matrix

**Date**: 2025-11-02
**Purpose**: Quick reference for file relocations during modular architecture migration
**Usage**: Use Ctrl+F to find old paths and see their new locations

---

## Quick Reference Summary

| Old Location | New Location | Notes |
|--------------|--------------|-------|
| `engine/src/*.cpp` | `engine/core/*` or `engine/modules/*` | See detailed tables below |
| `engine/include/*.h` | `engine/core/*` or `engine/modules/*` | Headers move with implementation |
| `engine/Libraries/include/*` | `third_party/vendored/*` | Header-only libs |
| `engine/Libraries/lib/*` | System/vcpkg installation | Pre-compiled binaries removed |
| `renders/` | `output/renders/` | User-generated outputs |
| N/A | `output/exports/` | New: Scene/model exports |
| N/A | `output/cache/` | New: Shader cache, thumbnails |

---

## Detailed File Mapping

### Core Application Layer

| Old Path | New Path | Module |
|----------|----------|--------|
| `engine/src/main.cpp` | `engine/core/application/main.cpp` | Core |
| `engine/src/application_core.cpp` | `engine/core/application/application_core.cpp` | Core |
| `engine/src/cli_parser.cpp` | `engine/core/application/cli_parser.cpp` | Core |
| `engine/src/render_settings.cpp` | `engine/core/application/render_settings.cpp` | Core |
| `engine/src/schema_validator.cpp` | `engine/core/application/schema_validator.cpp` | Core |
| `engine/src/camera_controller.cpp` | `engine/core/rendering/camera_controller.cpp` | Core |
| `engine/include/application_core.h` | `engine/core/application/application_core.h` | Core |
| `engine/include/cli_parser.h` | `engine/core/application/cli_parser.h` | Core |
| `engine/include/render_settings.h` | `engine/core/application/render_settings.h` | Core |
| `engine/include/schema_validator.h` | `engine/core/application/schema_validator.h` | Core |
| `engine/include/camera_controller.h` | `engine/core/rendering/camera_controller.h` | Core |
| `engine/include/help_text.h` | `engine/core/application/help_text.h` | Core |
| `engine/include/config_defaults.h` | `engine/core/application/config_defaults.h` | Core |
| `engine/include/colors.h` | `engine/core/rendering/colors.h` | Core |

### Core Scene Management

| Old Path | New Path | Module |
|----------|----------|--------|
| `engine/src/scene_manager.cpp` | `engine/core/scene/scene_manager.cpp` | Core |
| `engine/src/light.cpp` | `engine/core/scene/light.cpp` | Core |
| `engine/src/material.cpp` | `engine/core/scene/material.cpp` | Core |
| `engine/src/json_ops.cpp` | `engine/core/scene/json_ops.cpp` | Core |
| `engine/include/scene_manager.h` | `engine/core/scene/scene_manager.h` | Core |
| `engine/include/light.h` | `engine/core/scene/light.h` | Core |
| `engine/include/material.h` | `engine/core/scene/material.h` | Core |
| `engine/include/pbr_material.h` | `engine/core/scene/pbr_material.h` | Core |
| `engine/include/json_ops.h` | `engine/core/scene/json_ops.h` | Core |

### Core Rendering System

| Old Path | New Path | Module |
|----------|----------|--------|
| `engine/src/render_system.cpp` | `engine/core/rendering/render_system.cpp` | Core |
| `engine/src/shader.cpp` | `engine/core/rendering/shader.cpp` | Core |
| `engine/src/texture.cpp` | `engine/core/rendering/texture.cpp` | Core |
| `engine/src/texture_cache.cpp` | `engine/core/rendering/texture_cache.cpp` | Core |
| `engine/src/skybox.cpp` | `engine/core/rendering/skybox.cpp` | Core |
| `engine/src/ibl_system.cpp` | `engine/core/rendering/ibl_system.cpp` | Core |
| `engine/src/glad.c` | `engine/core/rendering/glad.c` | Core |
| `engine/include/render_system.h` | `engine/core/rendering/render_system.h` | Core |
| `engine/include/shader.h` | `engine/core/rendering/shader.h` | Core |
| `engine/include/Texture.h` | `engine/core/rendering/Texture.h` | Core |
| `engine/include/texture_cache.h` | `engine/core/rendering/texture_cache.h` | Core |
| `engine/include/skybox.h` | `engine/core/rendering/skybox.h` | Core |
| `engine/include/ibl_system.h` | `engine/core/rendering/ibl_system.h` | Core |
| `engine/include/gl_platform.h` | `engine/core/rendering/gl_platform.h` | Core |
| `engine/include/render_utils.h` | `engine/core/rendering/render_utils.h` | Core |

### Core I/O & Asset Loading

| Old Path | New Path | Module |
|----------|----------|--------|
| `engine/src/objloader.cpp` | `engine/core/io/objloader.cpp` | Core |
| `engine/src/mesh_loader.cpp` | `engine/core/io/mesh_loader.cpp` | Core |
| `engine/src/importer_registry.cpp` | `engine/core/io/importer_registry.cpp` | Core |
| `engine/src/importers/obj_importer.cpp` | `engine/core/io/importers/obj_importer.cpp` | Core |
| `engine/src/importers/assimp_importer.cpp` | `engine/core/io/importers/assimp_importer.cpp` | Core |
| `engine/src/assimp_loader.cpp` | `engine/core/io/assimp_loader.cpp` | Core |
| `engine/src/image_io.cpp` | `engine/core/io/image_io.cpp` | Core |
| `engine/src/resource_paths.cpp` | `engine/core/io/resource_paths.cpp` | Core |
| `engine/src/user_paths.cpp` | `engine/core/io/user_paths.cpp` | Core |
| `engine/src/path_security.cpp` | `engine/core/io/path_security.cpp` | Core |
| `engine/include/objloader.h` | `engine/core/io/objloader.h` | Core |
| `engine/include/mesh_loader.h` | `engine/core/io/mesh_loader.h` | Core |
| `engine/include/importer.h` | `engine/core/io/importer.h` | Core |
| `engine/include/importer_registry.h` | `engine/core/io/importer_registry.h` | Core |
| `engine/include/assimp_loader.h` | `engine/core/io/assimp_loader.h` | Core |
| `engine/include/image_io.h` | `engine/core/io/image_io.h` | Core |
| `engine/include/resource_paths.h` | `engine/core/io/resource_paths.h` | Core |
| `engine/include/user_paths.h` | `engine/core/io/user_paths.h` | Core |
| `engine/include/path_security.h` | `engine/core/io/path_security.h` | Core |

### Optional Module: Ray Tracing

| Old Path | New Path | Module |
|----------|----------|--------|
| `engine/src/raytracer.cpp` | `engine/modules/raytracing/raytracer.cpp` | Optional |
| `engine/src/BVHNode.cpp` | `engine/modules/raytracing/BVHNode.cpp` | Optional |
| `engine/src/triangle.cpp` | `engine/modules/raytracing/triangle.cpp` | Optional |
| `engine/src/RayUtils.cpp` | `engine/modules/raytracing/RayUtils.cpp` | Optional |
| `engine/src/brdf.cpp` | `engine/modules/raytracing/brdf.cpp` | Optional |
| `engine/src/microfacet_sampling.cpp` | `engine/modules/raytracing/microfacet_sampling.cpp` | Optional |
| `engine/src/raytracer_lighting.cpp` | `engine/modules/raytracing/raytracer_lighting.cpp` | Optional |
| `engine/src/refraction.cpp` | `engine/modules/raytracing/refraction.cpp` | Optional |
| `engine/include/raytracer.h` | `engine/modules/raytracing/raytracer.h` | Optional |
| `engine/include/BVHNode.h` | `engine/modules/raytracing/BVHNode.h` | Optional |
| `engine/include/triangle.h` | `engine/modules/raytracing/triangle.h` | Optional |
| `engine/include/ray.h` | `engine/modules/raytracing/ray.h` | Optional |
| `engine/include/RayUtils.h` | `engine/modules/raytracing/RayUtils.h` | Optional |
| `engine/include/brdf.h` | `engine/modules/raytracing/brdf.h` | Optional |
| `engine/include/microfacet_sampling.h` | `engine/modules/raytracing/microfacet_sampling.h` | Optional |
| `engine/include/raytracer_lighting.h` | `engine/modules/raytracing/raytracer_lighting.h` | Optional |
| `engine/include/refraction.h` | `engine/modules/raytracing/refraction.h` | Optional |

**CMake Option**: `GLINT_ENABLE_RAYTRACING` (default: ON)

### Optional Module: Editor Gizmos

| Old Path | New Path | Module |
|----------|----------|--------|
| `engine/src/grid.cpp` | `engine/modules/gizmos/grid.cpp` | Optional |
| `engine/src/axisrenderer.cpp` | `engine/modules/gizmos/axisrenderer.cpp` | Optional |
| `engine/src/gizmo.cpp` | `engine/modules/gizmos/gizmo.cpp` | Optional |
| `engine/include/grid.h` | `engine/modules/gizmos/grid.h` | Optional |
| `engine/include/axisrenderer.h` | `engine/modules/gizmos/axisrenderer.h` | Optional |
| `engine/include/gizmo.h` | `engine/modules/gizmos/gizmo.h` | Optional |

**CMake Option**: `GLINT_ENABLE_GIZMOS` (default: ON)

### Platform Layer: Desktop

| Old Path | New Path | Module |
|----------|----------|--------|
| `engine/src/ui_bridge.cpp` | `engine/platform/desktop/ui_bridge.cpp` | Platform |
| `engine/src/imgui_ui_layer.cpp` | `engine/platform/desktop/imgui_ui_layer.cpp` | Platform |
| `engine/src/file_dialog.cpp` | `engine/platform/desktop/file_dialog.cpp` | Platform |
| `engine/src/panels/scene_tree_panel.cpp` | `engine/platform/desktop/panels/scene_tree_panel.cpp` | Platform |
| `engine/imgui_impl_glfw.cpp` | `engine/platform/desktop/imgui_impl_glfw.cpp` | Platform |
| `engine/imgui_impl_opengl3.cpp` | `engine/platform/desktop/imgui_impl_opengl3.cpp` | Platform |
| `engine/include/ui_bridge.h` | `engine/platform/desktop/ui_bridge.h` | Platform |
| `engine/include/imgui_ui_layer.h` | `engine/platform/desktop/imgui_ui_layer.h` | Platform |
| `engine/include/file_dialog.h` | `engine/platform/desktop/file_dialog.h` | Platform |
| `engine/include/panels/scene_tree_panel.h` | `engine/platform/desktop/panels/scene_tree_panel.h` | Platform |
| `engine/include/input_handler.h` | `engine/platform/desktop/input_handler.h` | Platform |

### Third-Party Dependencies: Vendored (Header-Only)

| Old Path | New Path | Type |
|----------|----------|------|
| `engine/Libraries/include/glm/` | `third_party/vendored/glm/` | Math library |
| `engine/Libraries/include/stb/` | `third_party/vendored/stb/` | Image I/O |
| `engine/Libraries/include/rapidjson/` | `third_party/vendored/rapidjson/` | JSON parsing |
| `engine/Libraries/include/tinyexr.h` | `third_party/vendored/tinyexr/tinyexr.h` | EXR images |
| `engine/Libraries/include/miniz*.h` | `third_party/vendored/miniz/miniz*.h` | ZIP compression |
| `engine/Libraries/src/miniz*.c` | `third_party/vendored/miniz/miniz*.c` | ZIP compression |
| `engine/Libraries/include/imgui/` | `third_party/vendored/imgui/` | Desktop UI |
| `engine/Libraries/include/glad/` | `third_party/vendored/glad/` | OpenGL loader |
| `engine/Libraries/include/KHR/` | `third_party/vendored/glad/KHR/` | OpenGL headers |

### Third-Party Dependencies: Managed (System/vcpkg)

| Old Path | New Method | Package Name |
|----------|------------|--------------|
| `engine/Libraries/include/GLFW/` | vcpkg/system | glfw3 |
| `engine/Libraries/lib/glfw*.lib` | vcpkg/system | glfw3 |
| `engine/Libraries/include/OpenImageDenoise/` | vcpkg/system | openimagedenoise |
| `engine/Libraries/lib/OpenImageDenoise*.lib` | vcpkg/system | openimagedenoise |
| N/A (Assimp loaded at runtime) | vcpkg/system | assimp |

**Installation**:
```bash
vcpkg install glfw3 openimagedenoise assimp
```

### Output Directories

| Old Path | New Path | Purpose |
|----------|----------|---------|
| `renders/` | `output/renders/` | User-generated render outputs |
| N/A | `output/exports/` | Scene/model exports (JSON, glTF, etc.) |
| N/A | `output/cache/` | Shader cache, thumbnails, temp files |

**Git Configuration**:
```gitignore
# Ignore generated outputs but preserve structure
output/renders/*
!output/renders/.gitkeep
!output/renders/README.md
output/exports/*
!output/exports/.gitkeep
output/cache/*
!output/cache/.gitkeep
```

---

## CMake Variable Mapping

### Old Include Directories
```cmake
# OLD (Line 130-136, 145-152)
target_include_directories(glint PRIVATE
    engine/include
    engine
    engine/Libraries/include
    engine/Libraries/include/stb
    engine/Libraries/include/imgui
    engine/Libraries/include/imgui/backends
)
```

### New Include Directories
```cmake
# NEW (Modular)
target_include_directories(glint_core PUBLIC
    engine/include                      # Public API headers
    third_party/vendored/glm
    third_party/vendored/stb
    third_party/vendored/rapidjson
    third_party/vendored/imgui
    third_party/vendored/imgui/backends
    third_party/vendored/glad
)

target_include_directories(glint_core PRIVATE
    engine/core
    engine/modules
    engine/platform
)
```

---

## Migration Batches

To minimize risk, migrate files in these batches:

### Batch 1: Third-Party (Phase 3)
- Move `engine/Libraries/include/*` → `third_party/vendored/*`
- Update CMake include paths with transitional flag
- Test build

### Batch 2: Core I/O (Phase 4.1)
- Move `engine/src/*_loader.cpp` → `engine/core/io/`
- Move `engine/src/importer*` → `engine/core/io/`
- Move path utilities
- Test build

### Batch 3: Core Scene (Phase 4.2)
- Move `scene_manager.cpp`, `light.cpp`, `material.cpp`, `json_ops.cpp`
- Test build

### Batch 4: Core Rendering (Phase 4.3)
- Move `render_system.cpp`, `shader.cpp`, `texture*.cpp`
- Move `skybox.cpp`, `ibl_system.cpp`, `camera_controller.cpp`
- Test build

### Batch 5: Core Application (Phase 4.4)
- Move `main.cpp`, `application_core.cpp`, `cli_parser.cpp`
- Test build

### Batch 6: Ray Tracing Module (Phase 5.1)
- Move all `raytracer*`, `BVH*`, `triangle.cpp`, `brdf.cpp`, etc.
- Add CMake option `GLINT_ENABLE_RAYTRACING`
- Test build with and without option

### Batch 7: Gizmo Module (Phase 5.2)
- Move `grid.cpp`, `axisrenderer.cpp`, `gizmo.cpp`
- Add CMake option `GLINT_ENABLE_GIZMOS`
- Test build

### Batch 8: Platform Layer (Phase 6)
- Move `ui_bridge.cpp`, `imgui_ui_layer.cpp`, `file_dialog.cpp`
- Move `panels/scene_tree_panel.cpp`
- Test build

---

## Verification Commands

After each batch, run these verification commands:

```bash
# Clean build
rm -rf builds/desktop/cmake
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build builds/desktop/cmake --config Release

# Test executable
./builds/desktop/cmake/Release/glint.exe --help
./builds/desktop/cmake/Release/glint.exe --ops examples/json-ops/sphere_basic.json --render output/renders/test.png

# Check for missing files
git status | grep "deleted:"

# Check for broken includes
cmake --build builds/desktop/cmake 2>&1 | grep "error:"
```

---

## Common Issues & Solutions

### Issue: "Cannot find file: glm/glm.hpp"
**Cause**: Include path not updated after moving `engine/Libraries/include/glm`
**Solution**:
```cmake
target_include_directories(glint_core PUBLIC third_party/vendored/glm)
```

### Issue: "Unresolved external symbol: ImGui::Begin"
**Cause**: ImGui source files not added to build after move
**Solution**:
```cmake
set(IMGUI_SOURCES
    third_party/vendored/imgui/imgui.cpp
    third_party/vendored/imgui/imgui_draw.cpp
    third_party/vendored/imgui/imgui_tables.cpp
    third_party/vendored/imgui/imgui_widgets.cpp
)
target_sources(glint_core PRIVATE ${IMGUI_SOURCES})
```

### Issue: "Cannot open file: imgui_impl_glfw.h"
**Cause**: ImGui backend headers moved to platform layer
**Solution**:
```cmake
target_include_directories(glint_core PRIVATE engine/platform/desktop)
```

### Issue: Shaders not loading at runtime
**Cause**: `GLINT_RESOURCE_ROOT` still points to old location
**Solution**: `resources/` stays at project root, no change needed. Verify:
```cmake
target_compile_definitions(glint PRIVATE GLINT_RESOURCE_ROOT="${GLINT_RESOURCES_DIR_NORMALIZED}")
```

---

## Rollback Reference

If a batch fails, use these commands to rollback:

```bash
# Rollback last commit (batch)
git reset --hard HEAD~1

# Restore specific files
git checkout HEAD -- <file-path>

# Clean untracked files (new directories)
git clean -fd

# Rebuild from known-good state
rm -rf builds/
cmake -S . -B builds/desktop/cmake
cmake --build builds/desktop/cmake
```

---

## Post-Migration Grep Commands

After migration is complete, verify all old paths are removed:

```bash
# Should return no results:
grep -r "engine/Libraries" CMakeLists.txt
grep -r "renders/" --include="*.cpp" --include="*.h" --exclude-dir=output
grep -r "engine/src" CMakeLists.txt

# Should find only new paths:
grep -r "third_party/vendored" CMakeLists.txt
grep -r "engine/core" CMakeLists.txt
grep -r "engine/modules" CMakeLists.txt
```

---

**Matrix Completed**: 2025-11-02
**Total Mappings**: 88 files + 11 directories
**Next Step**: Execute Phase 1 (Establish Scaffolding)
