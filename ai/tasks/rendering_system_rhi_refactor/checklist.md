<!-- Machine Summary Block -->
{"file":"ai/tasks/rendering_system_rhi_refactor/checklist.md","purpose":"Tracks step-by-step progress for rendering system RHI refactoring and material unification.","exports":["task_phases","acceptance_criteria"],"depends_on":["task.json","CLAUDE.md"],"notes":["20_sequential_steps","4_week_timeline"]}
<!-- Human Summary -->
Checklist tracking the refactoring of Glint3D's rendering system from direct OpenGL calls to RHI abstraction with unified material system. Based on comprehensive audit findings and 20-step migration plan.

# Rendering System RHI Refactoring Checklist

## Phase 0: Audit and Planning (COMPLETED)
- [x] **Perform comprehensive architecture audit** - Document dual-pipeline architecture, identify 200+ direct OpenGL calls in render_system.cpp, catalog material duplication patterns, and find legacy code for removal.
- [x] **Analyze material system issues** - Document dual storage in SceneObject (Material + PBR fields), asymmetric transmission handling, and synchronization complexity in json_ops.cpp.
- [x] **Identify classes needing decomposition** - RenderSystem (1459 lines), SceneObject dual storage, Shader class GL coupling, and legacy Material/PBRMaterial structs.
- [x] **Create task module** - Establish task structure with acceptance criteria, phases, artifacts, and documentation following project standards.

## Phase 1: Foundation (Tasks 1-3)
- [ ] **Define RHI Interface** - Create `engine/core/rendering/rhi/RHI.h` abstract interface for GPU operations (draw, texture, framebuffer, shader) with platform-agnostic types.
- [ ] **Implement RhiGL Backend** - Create `engine/core/rendering/rhi/RhiGL.cpp` wrapping existing OpenGL calls with error handling and state validation.
- [ ] **Thread Rasterizer Through RHI** - Replace direct OpenGL calls in render_system.cpp (200+ calls) with RHI interface calls; verify golden images match with SSIM >= 0.995.

## Phase 2: Unified Materials (Tasks 4-5)
- [ ] **Introduce MaterialCore Struct** - Create single unified material struct in `engine/core/scene/material_core.h` with properties for both pipelines (baseColor, metallic, roughness, ior, transmission, clearcoat).
- [ ] **Adapt Raytracer to MaterialCore** - Update raytracer.cpp to use MaterialCore directly, removing PBR-to-legacy material conversion logic and eliminating conversion drift.

## Phase 3: Pass System (Tasks 6-7)
- [ ] **Add Minimal RenderGraph** - Create `engine/core/rendering/render_graph.h` with RenderPass interface (setup/execute/teardown) and PassContext for resource sharing.
- [ ] **Wire Material Uniforms to Shaders** - Add transmission uniform to pbr.frag, prepare shader infrastructure for screen-space refraction, and ensure MaterialCore properties map correctly to shader uniforms.

## Phase 4: Screen-Space Refraction (Tasks 8-9)
- [ ] **Implement SSR-T in PBR Shader** - Add screen-space refraction logic to pbr.frag with Snell's law refraction direction, Fresnel mixing, and scene color buffer sampling.
- [ ] **Add Roughness-Aware Blur** - Implement blur kernel in shader for frosted glass effect based on material roughness, approximating micro-surface scattering.

## Phase 5: Hybrid Pipeline (Tasks 10-12)
- [ ] **Implement Auto Mode with CLI** - Add `--mode raster|ray|auto` flag; auto mode analyzes scene materials and selects appropriate pipeline (ray if transmission > 0.01 and ior > 1.05).
- [ ] **Align BSDF Models** - Ensure consistent shading between GPU (pbr.frag) and CPU (brdf.cpp) BRDF implementations for predictable appearance across pipelines.
- [ ] **Add Clearcoat and Attenuation** - Extend MaterialCore with clearcoat layer and Beer-Lambert volumetric attenuation for advanced glass and coating materials.

## Phase 6: Production Features (Tasks 13-15)
- [ ] **Implement Auxiliary Readback** - Add depth, normals, and instance ID buffer exports for compositing workflows and debugging; expose via JSON ops.
- [ ] **Ensure Deterministic Rendering** - Add seeded random number generation for raytracer, stable sort for transparency, and render metadata output for reproducibility.
- [ ] **Create Golden Test Suite** - Establish 10+ canonical test scenes covering materials (glass, metal, plastic), lighting (point, directional, spot), and camera presets with SSIM validation.

## Phase 7: Platform Support (Tasks 16-18)
- [ ] **Verify WebGL2 Compliance** - Test RHI abstraction with WebGL2 backend, ensure ES 3.0 shader compatibility, handle precision qualifiers and texture format limitations.
- [ ] **Add Performance Profiling** - Implement per-pass timing hooks in RenderGraph, expose metrics via performance HUD, and log frame budget violations.
- [ ] **Implement Error Handling** - Add graceful fallbacks for missing features (SSR-T on low-end hardware), validate shader compilation across platforms, and provide clear error messages.

## Phase 8: Cleanup and Documentation (Tasks 19-20)
- [ ] **Write Architecture Documentation** - Create developer guide for RHI usage patterns, MaterialCore property reference, RenderGraph pass authoring, and backend porting instructions.
- [ ] **Remove Legacy Code** - Delete Material class, PBRMaterial struct, shadow mapping stubs (shadow_depth.frag/vert, dummy texture), deprecated JSON ops, and legacy texture fallback patterns.

## Validation Gates
- [ ] **Desktop Build Matrix** - Verify Release and Debug builds succeed on Windows, Linux, macOS with both default and minimal configurations (GLINT_ENABLE_RAYTRACING=OFF).
- [ ] **Web Build Verification** - Confirm Emscripten build produces working WASM module with RHI abstraction and MaterialCore functioning correctly in browser.
- [ ] **Golden Image Regression** - All reference scenes render with SSIM >= 0.995 compared to baseline images; no visual artifacts introduced by refactoring.
- [ ] **Performance Benchmarks** - RHI overhead < 5%, frame times at 1080p remain < 16ms for standard scenes, memory usage delta < 10% compared to pre-refactor baseline.
- [ ] **Code Quality** - No direct OpenGL calls outside RHI layer, no dual material storage in SceneObject, all legacy code removed, and documentation updated.
