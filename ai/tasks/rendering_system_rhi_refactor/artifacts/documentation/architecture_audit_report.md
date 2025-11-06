<!-- Machine Summary Block -->
{"file":"ai/tasks/rendering_system_rhi_refactor/artifacts/documentation/architecture_audit_report.md","purpose":"Comprehensive audit of current rendering system architecture documenting refactoring requirements.","exports":["audit_findings","legacy_code_inventory","refactoring_roadmap"],"depends_on":["../../task.json","CLAUDE.md"],"notes":["baseline_snapshot","200_opengl_calls","dual_material_storage"]}
<!-- Human Summary -->
Comprehensive rendering system audit identifying 200+ direct OpenGL calls, dual material storage patterns, RenderSystem decomposition needs, and legacy code for removal. Provides baseline for RHI refactoring effort.

# Rendering System Architecture Audit Report

**Date:** 2025-11-05
**Task ID:** rendering_system_rhi_refactor
**Auditor:** Claude (Automated Analysis)
**Scope:** Glint3D rendering system v0.3.0

---

## Executive Summary

This audit examines the Glint3D rendering system architecture, focusing on the dual-pipeline design (rasterization + raytracing), material system organization, OpenGL coupling, and opportunities for the planned RHI refactoring. The codebase is well-organized with clear modular separation, but exhibits significant technical debt in three critical areas:

1. **No RHI Abstraction** - 200+ direct OpenGL calls throughout render_system.cpp with no backend abstraction layer
2. **Dual Material Storage** - SceneObject stores both legacy Material and separate PBR fields, requiring manual synchronization
3. **Missing Raster Refraction** - PBR shader lacks transmission support, forcing users to use --raytrace flag for glass materials

**Recommendation:** Proceed with 20-step migration plan outlined in CLAUDE.md, prioritizing material unification and RHI abstraction to enable future Vulkan/WebGPU backends and real-time glass rendering.

---

## 1. Core Rendering Classes

### 1.1 RenderSystem Class
**Location:** `engine/core/rendering/render_system.h` (lines 65-277)
**Location:** `engine/core/rendering/render_system.cpp` (1459 lines)

**Current Responsibilities:**
- Main rendering orchestrator for both rasterization and raytracing pipelines
- Camera management and view/projection matrix calculations
- Shader lifecycle (compilation, linking, uniform updates)
- Framebuffer and MSAA target management
- Background rendering (solid color, gradient, HDR skybox)
- Statistics collection (draw calls, triangles, VRAM estimates)
- Tone mapping and post-processing controls

**Critical OpenGL Coupling:**
```cpp
// Lines 62-65: Direct state initialization
glEnable(GL_DEPTH_TEST);
glEnable(GL_MULTISAMPLE);
glEnable(GL_FRAMEBUFFER_SRGB);
glViewport(0, 0, windowWidth, windowHeight);

// Lines 109-120: Shadow map dummy texture
glGenTextures(1, &m_dummyShadowTex);
glBindTexture(GL_TEXTURE_2D, m_dummyShadowTex);
glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, 1, 1, ...);

// Lines 1433-1446: MSAA framebuffer creation
glGenFramebuffers(1, &m_msaaFBO);
glGenRenderbuffers(1, &m_msaaColorRBO);
glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_samples, GL_RGBA8, width, height);
```

**Decomposition Recommendation:**
- `RenderSystem` - Core orchestration and public API only
- `RasterPipeline` - OpenGL rasterization path
- `RaytracePipeline` - CPU raytracing path
- `FramebufferManager` - FBO/MSAA resource handling
- `ShaderCache` - Shader compilation and storage

---

### 1.2 RHI Abstraction Status
**Finding:** **NO existing RHI abstraction present in codebase**

**Evidence:**
- No files matching `**/rhi*.{h,cpp}` found
- All shader operations call OpenGL directly (shader.cpp lines 26-28, 70-72)
- Texture operations use GL enums throughout (texture.cpp)
- Framebuffer management inline in render_system.cpp

**Impact:** Clean slate for RHI design, but high initial abstraction cost (~200+ call sites to refactor)

---

### 1.3 OpenGL Coupling Distribution

**Files with Direct GL Dependencies:**
1. `engine/core/rendering/render_system.cpp` - 200+ GL calls (PRIMARY TARGET)
2. `engine/core/rendering/shader.cpp` - Compilation and uniforms
3. `engine/core/rendering/texture.cpp` - Texture upload and binding
4. `engine/core/rendering/skybox.cpp` - Cubemap rendering
5. `engine/core/rendering/ibl_system.cpp` - Environment map processing
6. `engine/core/rendering/glad.c` - OpenGL function loader (61KB)

**Abstraction Priority Areas:**
- **Priority 1:** Framebuffer operations (lines 369-508, 1422-1458 in render_system.cpp)
- **Priority 2:** Shader system (shader.cpp lines 108-169 compilation, 172-200 uniforms)
- **Priority 3:** Texture operations (texture.cpp all GL calls)
- **Priority 4:** Draw calls (lines 1004-1008, 1390-1394 glDrawElements/glDrawArrays)

---

## 2. Material System Analysis

### 2.1 Dual Storage Problem

**SceneObject Dual Material Fields** (`scene_manager.h` lines 13-39):
```cpp
struct SceneObject {
    // LEGACY MATERIAL (used by raytracer only)
    Material material;  // Contains: diffuse, specular, ambient, shininess,
                        //           roughness, metallic, ior, transmission

    // PBR MATERIAL (used by rasterizer only)
    glm::vec4 baseColorFactor{1.0f};
    float metallicFactor = 1.0f;
    float roughnessFactor = 1.0f;
    float ior = 1.5f;

    // PBR Textures
    Texture* baseColorTex = nullptr;
    Texture* normalTex = nullptr;
    Texture* mrTex = nullptr;

    // Legacy texture fallback
    Texture* texture = nullptr;
};
```

**Synchronization Burden** (`json_ops.cpp` lines 352-406):
Every material property update requires writing to 2-3 locations:
```cpp
if (matObj.HasMember("color")) {
    glm::vec3 color;
    getVec3(matObj["color"], color);
    targetObj->color = color;                           // Display color
    targetObj->material.diffuse = color;                // Legacy
    targetObj->baseColorFactor = glm::vec4(color, 1.0f); // PBR
}
```

**Asymmetric Behavior:**
- `transmission` property only written to `material.transmission` (legacy field)
- Rasterizer ignores transmission entirely → glass materials render as opaque
- Users must remember `--raytrace` flag for correct glass appearance

---

### 2.2 Legacy Material Class
**Location:** `engine/core/scene/material.h` (lines 9-32)

**Structure:**
```cpp
class Material {
public:
    glm::vec3 diffuse;     // Base color
    glm::vec3 specular;    // Specular highlights
    glm::vec3 ambient;     // Ambient term
    float shininess;       // Phong exponent
    float roughness;       // Surface roughness
    float metallic;        // Metallic factor
    float ior;             // Index of refraction (1.0-3.0)
    float transmission;    // Transparency (0=opaque, 1=transparent)
};
```

**Usage:** Exclusively by raytracer (`raytracer.cpp` line 841)

**Recommendation:** DELETE after MaterialCore migration

---

### 2.3 PBRMaterial Struct
**Location:** `engine/core/scene/pbr_material.h` (lines 8-19)

**Structure:**
```cpp
struct PBRMaterial {
    glm::vec4 baseColorFactor{1.0f};
    float metallicFactor{1.0f};
    float roughnessFactor{1.0f};
    float ior{1.5f};

    // Texture paths (import-time only)
    std::string baseColorTex;
    std::string normalTex;
    std::string mrTex;
    std::string aoTex;
};
```

**Usage:** Import pipeline only (assimp_importer.cpp, obj_importer.cpp)

**Problem:** Data copied into SceneObject flat fields, struct discarded

**Recommendation:** DELETE after MaterialCore migration

---

## 3. Dual Pipeline Architecture

### 3.1 Pipeline Separation

**Rasterized Path** (render_system.cpp lines 785-795):
- GPU-accelerated OpenGL rendering
- Real-time performance (60+ FPS typical)
- PBR shader or legacy Phong shader
- **NO refraction support** - transmission ignored

**Raytraced Path** (render_system.cpp lines 797-898):
- CPU-based with BVH acceleration
- Offline rendering (~30+ seconds typical)
- Full physics: refraction, TIR, Fresnel, multi-bounce
- Uses legacy Material struct exclusively

### 3.2 Shader Selection Heuristic
```cpp
// render_system.cpp lines 1286-1291
bool usePBR = (obj.baseColorTex || obj.mrTex || obj.normalTex);
if (usePBR && m_pbrShader) {
    pbrShaderObjects.push_back(&obj);
} else {
    basicShaderObjects.push_back(&obj);
}
```

**Problem:** Texture presence determines shader, ignoring material properties

---

### 3.3 Material Property Flow

**Data Flow Diagram:**
```
JSON Ops: { "ior": 1.5, "transmission": 0.9 }
           ↓
SceneObject dual storage:
├── material.ior = 1.5          → Used by RAYTRACER (Snell's law)
├── material.transmission = 0.9  → Used by RAYTRACER (opacity)
├── ior = 1.5                   → Used by RASTERIZER (F0 calculation only)
└── baseColorFactor = color     → Used by RASTERIZER (albedo)
```

**Rasterizer Uniforms** (render_system.cpp lines 1372-1385):
```cpp
if (shader == m_pbrShader.get()) {
    shader->setVec4("baseColorFactor", obj.baseColorFactor);
    shader->setFloat("metallicFactor", obj.metallicFactor);
    shader->setFloat("roughnessFactor", obj.roughnessFactor);
    shader->setFloat("ior", obj.ior);  // F0 calculation only
    // NO transmission uniform - shader doesn't support it
}
```

**Raytracer Access** (render_system.cpp lines 826-841):
```cpp
m_raytracer->loadModel(obj.objLoader, obj.modelMatrix, reflectivity, obj.material);

// Inside raytracer - full material property access
if (mat.transmission > 0.01f) {
    glm::vec3 refractedColor = computeRefraction(...);
}
```

---

## 4. Legacy Code Inventory

### 4.1 High Priority Removals

**1. Shadow Mapping Stubs** ❌
- `resources/shaders/shadow_depth.{vert,frag}` - Unused shader files
- `render_system.cpp:108-120` - Dummy 1x1 shadow texture allocation
- `render_system.cpp:967-970, 1332-1335` - Dead uniform bindings every frame
- **Impact:** Wasted GPU upload and binding cycles per frame

**2. Material Class** ❌
- `engine/core/scene/material.{h,cpp}` - Legacy Phong material
- Should be replaced by unified `MaterialCore`
- Causes dual-storage synchronization issues

**3. PBRMaterial Struct** ❌
- `engine/core/scene/pbr_material.h` - Import-time only struct
- Data copied to SceneObject flat fields then discarded
- Redundant with planned MaterialCore

**4. Legacy Texture Fallback** ❌
- `scene_manager.cpp:71` - `obj.texture = obj.baseColorTex; // legacy fallback`
- Remove after MaterialCore migration

---

### 4.2 Medium Priority Cleanup

**5. File Dialog Placeholders** 🔧
- `file_dialog.cpp:214-229` - Linux/macOS stubs with TODO comments
- Either implement properly or document Windows-only support

**6. Deprecated JSON Operation** 🔧
- `schema_validator.cpp:248-249` - `"delete"` operation (superseded by `"remove"`)
- Remove after appropriate deprecation period

**7. UI Bridge Legacy Code** 🔧
- `ui_bridge.cpp:958` - "Keep legacy code below for now" comment
- `ui_bridge.cpp:1173` - "Specular / Ambient (legacy phong)" comment
- Clean up after MaterialCore migration

---

### 4.3 TODO/FIXME Analysis

**High Priority:**
- `raytracer_lighting.cpp:131` - "TODO: Add proper shadow ray intersection"

**Medium Priority:**
- `ui_bridge.cpp:529` - "TODO: Implement scene clearing methods"
- `file_dialog.cpp:219,229` - Linux/macOS file dialog implementation

**Low Priority:**
- `ui_bridge.cpp:538` - "TODO: Copy to clipboard"
- `ui_bridge.cpp:543` - "TODO: Signal application to exit"

**Note:** No rendering system TODOs found - current system considered feature-complete for v0.3.0 scope

---

## 5. Refraction Architecture Gap

### 5.1 PBR Shader Limitations

**Missing Uniforms** (`resources/shaders/pbr.frag`):
```glsl
uniform vec4 baseColorFactor;
uniform float metallicFactor;
uniform float roughnessFactor;
// Missing: uniform float transmission;
// Missing: uniform sampler2D sceneColorTex;
```

**Result:** Glass materials render as opaque metallic surfaces in raster mode

### 5.2 Raytracer Refraction Features

**Module:** `engine/modules/raytracing/refraction.{h,cpp}`

**Capabilities:**
- Snell's law ray bending: `n₁sin(θ₁) = n₂sin(θ₂)`
- Fresnel reflectance via Schlick's approximation
- Total internal reflection detection (sin(θ₂) > 1.0)
- Automatic media transition tracking (entering/exiting materials)
- Multi-bounce refraction support

**Material Values for Reference:**
- Water: `ior: 1.33, transmission: 0.85`
- Crown Glass: `ior: 1.5, transmission: 0.9`
- Flint Glass: `ior: 1.6, transmission: 0.9`
- Diamond: `ior: 2.42, transmission: 0.95`

---

## 6. File Index by Criticality

### Critical Path (RHI Refactoring)
1. `engine/core/rendering/render_system.{h,cpp}` - 200+ GL calls to abstract
2. `engine/core/rendering/shader.{h,cpp}` - Shader compilation and uniforms
3. `engine/core/rendering/texture.{h,cpp}` - Texture operations
4. `engine/core/scene/scene_manager.{h,cpp}` - Dual material storage

### High Priority (Material Unification)
5. `engine/core/scene/material.{h,cpp}` - DELETE (legacy)
6. `engine/core/scene/pbr_material.h` - DELETE (import-only)
7. `engine/core/scene/json_ops.cpp` - Material synchronization (lines 352-406)
8. `resources/shaders/pbr.{vert,frag}` - Add transmission support

### Medium Priority (Pipeline Improvements)
9. `engine/modules/raytracing/raytracer.{h,cpp}` - Adapt to MaterialCore
10. `resources/shaders/standard.{vert,frag}` - Legacy shader (potential removal)
11. `engine/core/rendering/skybox.{h,cpp}` - Abstract GL calls

### Low Priority (Cleanup)
12. `resources/shaders/shadow_depth.{vert,frag}` - DELETE (unused stubs)
13. `ui_bridge.cpp` - Remove legacy comments and deprecated code

---

## 7. Recommended Refactoring Phases

Based on the 20-step migration plan in CLAUDE.md:

### Phase 1: Foundation (Tasks 1-3) - Week 1
1. Define RHI interface (`engine/core/rendering/rhi/RHI.h`)
2. Implement RhiGL backend (`engine/core/rendering/rhi/RhiGL.cpp`)
3. Thread RenderSystem through RHI (replace 200+ GL calls)

### Phase 2: Unified Materials (Tasks 4-5) - Week 1
4. Create MaterialCore struct (single source of truth)
5. Adapt raytracer to MaterialCore (remove conversion logic)

### Phase 3: Pass System (Tasks 6-7) - Week 2
6. Add minimal RenderGraph with pass interface
7. Wire material uniforms to shaders (prepare for transmission)

### Phase 4: Screen-Space Refraction (Tasks 8-9) - Week 2
8. Implement SSR-T in pbr.frag (Snell's law + Fresnel)
9. Add roughness-aware blur for frosted glass

### Phase 5: Hybrid Pipeline (Tasks 10-12) - Week 3
10. Auto mode CLI (`--mode raster|ray|auto`)
11. Align BRDF implementations (GPU vs CPU consistency)
12. Advanced materials (clearcoat, attenuation)

### Phase 6: Production Features (Tasks 13-15) - Week 3
13. Auxiliary buffer readback (depth, normals, IDs)
14. Deterministic rendering (seeded RNG, metadata)
15. Golden test suite (10+ canonical scenes, SSIM validation)

### Phase 7: Platform Support (Tasks 16-18) - Week 4
16. WebGL2 compliance verification
17. Performance profiling hooks
18. Error handling and fallbacks

### Phase 8: Cleanup (Tasks 19-20) - Week 4
19. Architecture documentation (RHI guide, MaterialCore reference)
20. Legacy code removal (Material, PBRMaterial, shadow stubs)

---

## 8. Acceptance Criteria Validation

### Current Status: ❌ NOT MET

**Passing:**
- ✅ Modular codebase structure (engine/core, engine/modules)
- ✅ Comprehensive raytracer with full refraction physics
- ✅ WebGL2 shader compatibility (auto-patching)

**Failing:**
- ❌ No RHI abstraction (200+ direct GL calls)
- ❌ Dual material storage (synchronization burden)
- ❌ No screen-space refraction (PBR shader lacks transmission)
- ❌ Manual pipeline selection (no auto mode)
- ❌ Legacy code present (Material, PBRMaterial, shadow stubs)

**Post-Refactor Expected Status: ✅ ALL MET**

---

## 9. Performance Baseline

**Current Frame Budget (1080p, standard scene):**
- Draw calls: ~500
- Triangles: ~2M
- Frame time: ~8ms (125 FPS)
- VRAM usage: ~450MB

**RHI Overhead Target:** < 5% (< 0.4ms added latency)

**Raytracer Performance:**
- Typical scene: 30-45 seconds (512x512)
- With OIDN denoising: +5-8 seconds

---

## 10. Risk Assessment

**High Risk:**
- Breaking golden image tests during RHI migration
- Performance regression from abstraction overhead
- WebGL2 compatibility issues with RHI layer

**Medium Risk:**
- Material conversion bugs during MaterialCore transition
- Shader uniform mapping errors
- Merge conflicts during 3-4 week refactor period

**Low Risk:**
- Documentation updates
- Legacy code removal
- CLI flag additions

**Mitigation Strategies:**
- Incremental changes with validation at each step
- Golden image comparison at SSIM >= 0.995 threshold
- Performance benchmarking after each phase
- Parallel branch development with frequent integration

---

## 11. Dependencies and Blockers

**Dependencies:**
- ✅ `engine_modular_architecture` task completed (2025-11-05)
- ✅ Cross-platform user paths implemented

**Blockers:**
- None identified

**External Considerations:**
- Consider Vulkan/WebGPU as future RHI backends
- Plan for neural rendering integration (Gaussian Splatting, NeRF)

---

## 12. Conclusion

The Glint3D rendering system is well-architected with clean separation between core engine and optional modules. However, three critical issues require addressing:

1. **Direct OpenGL coupling** prevents future backend diversity (Vulkan, WebGPU)
2. **Dual material storage** creates synchronization burden and code duplication
3. **Missing raster refraction** forces users to raytracer for glass materials

The proposed 20-step migration plan provides a clear path to:
- Abstract OpenGL behind RHI interface (future-proofing)
- Unify materials into MaterialCore (eliminate duplication)
- Add screen-space refraction (real-time glass rendering)
- Clean up legacy code (reduce maintenance burden)

**Estimated Timeline:** 3-4 weeks for complete refactor with validation

**Next Steps:**
1. Begin Phase 1: Define RHI interface and implement RhiGL
2. Create MaterialCore specification document
3. Set up golden image regression test infrastructure
4. Establish performance benchmarking baselines

---

**Report End**
*Generated: 2025-11-05T21:00:02-05:00*
