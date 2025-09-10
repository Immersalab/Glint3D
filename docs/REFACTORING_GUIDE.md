# Glint3D Rendering System Refactoring Guide

**Target Audience**: Graphics engineers new to the Glint3D codebase  
**Goal**: Migrate from dual-pipeline architecture to unified RHI-based system  
**Timeline**: ~3.5 weeks for complete refactor

## 0. Quick Start - Understanding the Current System

### Build & Run (Desktop)
```bash
# From repo root
cmake -S . -B builds/desktop/cmake/Debug -DCMAKE_BUILD_TYPE=Debug
cmake --build builds/desktop/cmake/Debug --config Debug

# Launch interactive UI
./builds/desktop/cmake/Debug/glint.exe

# Headless render (raster default - no refraction)
./builds/desktop/cmake/Debug/glint.exe --ops examples/json-ops/sphere_basic.json --render out.png

# Headless render (raytracer - full refraction)
./builds/desktop/cmake/Debug/glint.exe --ops examples/json-ops/glass-sphere-refraction.json --render out.png --raytrace
```

### Codebase Structure
```
engine/
├── include/               # Headers: scene, material, render system
├── src/                   # Implementation: raster pipeline, CLI, JSON ops
├── shaders/               # GLSL: pbr.frag, standard.frag, .vert files
└── src/                   # CPU raytracer: raytracer.cpp, refraction.cpp

examples/json-ops/         # Sample scenes for testing
docs/                      # Documentation (you'll add rendering_arch_v1.md)
```

## 1. Current (Old) System - What You're Inheriting

### Two Completely Separate Pipelines

#### **Rasterized Pipeline (Default)**
- **API**: OpenGL/WebGL2 direct calls
- **Entry Point**: `engine/src/render_system.cpp`
- **Shaders**: `engine/shaders/standard.vert|frag`, `engine/shaders/pbr.vert|frag`
- **Materials**: PBR (baseColor, metallic, roughness) or legacy Blinn-Phong
- **Transparency**: Alpha blending only - **NO REFRACTION**
- **IOR Usage**: Only computes F0 = ((n-1)/(n+1))² for basic Fresnel
- **Performance**: Real-time (< 1 second)

#### **Raytraced Pipeline (--raytrace flag)**
- **API**: CPU path tracer with BVH acceleration
- **Entry Point**: `engine/src/raytracer.cpp`
- **Features**: Full reflection, refraction (Snell's law), Fresnel, Total Internal Reflection
- **Materials**: Converts PBR → "legacy Material" struct
- **Performance**: Offline (30+ seconds for test scenes)
- **Files**: `engine/src/raytracer.cpp`, `engine/src/refraction.cpp`

### Material Storage Problem (Critical Issue)

**SceneObject currently holds TWO material representations:**

```cpp
struct SceneObject {
    // Legacy Material (used by raytracer)
    Material material;
    ├── glm::vec3 diffuse, specular, ambient;
    ├── float shininess, roughness, metallic;
    ├── float ior;              // Used for Snell's law
    └── float transmission;     // Used for transparency
    
    // PBR fields (used by rasterizer)  
    glm::vec4 baseColorFactor;  // sRGB + alpha
    float metallicFactor;       // 0.0-1.0
    float roughnessFactor;      // 0.0-1.0
    float ior;                  // Used ONLY for F0 calculation
};
```

**Problems with this approach:**
- Material properties can diverge between pipelines
- PBR→legacy conversion adds maintenance burden  
- `transmission` is completely ignored by rasterizer
- No unified BSDF model

### Known Limitations

1. **Raster cannot do refraction** - No screen-space refraction or transmission model
2. **Material duplication** - PBR + legacy causing sync issues
3. **Backend lock-in** - Direct OpenGL calls, hard to port to Vulkan/WebGPU
4. **No pass system** - Pipeline steps are interleaved, hard to extend
5. **Manual mode selection** - Users must remember `--raytrace` for glass

## 2. Target (New) Architecture - What We're Building

### Design Goals
- **AI-accessible**: JSON Ops v2 + REST /jobs wrapper for deterministic, idempotent operations
- **Renderer focus**: Not a DCC tool - optimize for headless CLI and web deployment
- **Backend agnostic**: GL/WebGL2 now → Vulkan/WebGPU later via clean RHI abstraction
- **Single material truth**: MaterialCore BSDF drives both raster and ray (no conversions)
- **Hybrid auto mode**: Raster+SSR-T for previews, ray for finals when scene needs true physics
- **Lightweight & scalable**: Two builds - web-preview (2MB WASM) and desktop-final (full features)

### External Dependencies & Toolchain

The refactored system leverages proven, lightweight packages organized in `engine/external/`:

#### **Core Dependencies (Always Required)**
- **fmt + spdlog**: Fast, structured logging with minimal overhead
- **cgltf**: Lightweight, robust glTF 2.0 loader (prefer over heavy Assimp)
- **tinyexr**: HDR/floating-point image support for environment maps
- **GLM**: Already integrated - continue using for all math

#### **Shader Pipeline (Future-Proofing)**
- **shaderc**: GLSL → SPIR-V compilation for cross-platform shaders
- **SPIRV-Cross**: SPIR-V → GLSL ES/MSL/HLSL cross-compilation  
- **SPIRV-Reflect**: Automatic UBO layout detection and binding inference

#### **Backend Abstraction (Future)**
- **volk**: Vulkan function loader (when adding Vulkan backend)
- **VMA**: Vulkan Memory Allocator (painless memory management)
- **Dawn** or **wgpu-native**: WebGPU desktop implementation

#### **Optional Acceleration (Desktop Only)**
- **Intel Embree**: 2-10x faster CPU ray tracing vs custom BVH
- **Intel OIDN**: Already integrated - AI-based denoising for ray tracing

#### **Build Strategy**
```cmake
# Web Preview Build (Minimal WASM)
GLINT_WEB_PREVIEW=ON      # <2MB WASM, raster+SSR-T only
GLINT_ENABLE_EMBREE=OFF   # No CPU ray tracing
GLINT_ENABLE_OIDN=OFF     # No denoising

# Desktop Final Build (Full Features)  
GLINT_ENABLE_EMBREE=ON    # Fast CPU ray tracing
GLINT_ENABLE_OIDN=ON      # AI denoising
GLINT_ENABLE_SPIRV=ON     # Future shader pipeline
```

### Core Components

#### **RHI (Render Hardware Interface)**
```cpp
// Thin abstraction for GPU operations
class RHI {
public:
    virtual bool init(const RhiInit& desc) = 0;
    virtual void beginFrame() = 0;
    virtual void draw(const DrawDesc& desc) = 0;
    virtual void readback(const ReadbackDesc& desc) = 0;
    virtual void endFrame() = 0;
    
    // Resource management
    virtual TextureHandle createTexture(const TextureDesc& desc) = 0;
    virtual BufferHandle createBuffer(const BufferDesc& desc) = 0;
    virtual ShaderHandle createShader(const ShaderDesc& desc) = 0;
    virtual PipelineHandle createPipeline(const PipelineDesc& desc) = 0;
};

// Concrete implementations:
class RhiGL : public RHI { /* OpenGL backend */ };
class RhiWebGL2 : public RHI { /* WebGL2 backend */ };
// Future: RhiVulkan, RhiWebGPU
```

#### **MaterialCore (Unified BSDF)**
```cpp
struct MaterialCore {
    glm::vec4 baseColor;           // sRGB base color + alpha
    float metallic;                // 0=dielectric, 1=metal
    float roughness;               // 0=mirror, 1=rough
    float normalStrength;          // Normal map intensity [0,2]
    glm::vec3 emissive;           // Self-emission (linear RGB)
    float ior;                     // Index of refraction [1.0, 3.0]
    float transmission;            // Transparency factor [0,1]
    float thickness;               // Volume thickness (meters)
    float attenuationDistance;     // Beer-Lambert falloff distance
    float clearcoat;               // Clear coat layer strength [0,1]
    float clearcoatRoughness;      // Clear coat roughness [0,1]
    
    // Future extensions:
    // float subsurface;           // SSS strength
    // glm::vec3 subsurfaceColor;  // SSS tint
    // float anisotropy;           // Anisotropic roughness
};

// This SINGLE struct is used by BOTH pipelines - no conversion needed
```

#### **RenderGraph (Minimal Pass System)**
```cpp
class RenderPass {
public:
    virtual void setup(const PassContext& ctx) = 0;
    virtual void execute(const PassContext& ctx) = 0;
    virtual void teardown(const PassContext& ctx) = 0;
};

// Raster pipeline passes:
class GBufferPass : public RenderPass { /* Geometry + materials to G-buffer */ };
class LightingPass : public RenderPass { /* Deferred lighting */ };
class SSRRefractionPass : public RenderPass { /* Screen-space refraction */ };
class PostPass : public RenderPass { /* Tone mapping, gamma */ };
class ReadbackPass : public RenderPass { /* CPU readback */ };

// Ray pipeline passes:
class IntegratorPass : public RenderPass { /* Path tracing */ };
class DenoisePass : public RenderPass { /* OIDN denoising */ };
class TonemapPass : public RenderPass { /* HDR → LDR */ };
```

#### **Hybrid Auto Mode**
```cpp
enum class RenderMode { Raster, Ray, Auto };

class ModeSelector {
public:
    RenderMode selectMode(const Scene& scene, const RenderConfig& config) {
        if (config.mode != RenderMode::Auto) return config.mode;
        
        // Auto mode heuristics:
        for (const auto& material : scene.materials) {
            if (material.transmission > 0.01f && 
                (material.thickness > 0.0f || material.ior > 1.05f)) {
                return config.isPreview ? RenderMode::Raster : RenderMode::Ray;
            }
        }
        return RenderMode::Raster; // Default to faster raster
    }
};

// CLI usage:
// --mode raster  → Force raster (fast, SSR approximation)
// --mode ray     → Force ray (slow, physically accurate)  
// --mode auto    → Smart selection based on scene content
```

## 3. Migration Plan - 20 Sequential Tasks

Each task is designed to be a single PR with clear acceptance criteria.

### **Phase 1: Foundation (Week 1, Days 1-3)**

#### **Task 1: Define RHI Interface**
**Files**: `engine/rhi/RHI.h` (new), `engine/rhi/types.h` (new)

**What to do:**
```cpp
// Create abstract RHI interface
class RHI {
public:
    virtual ~RHI() = default;
    virtual bool init(const RhiInit& desc) = 0;
    virtual void beginFrame() = 0;
    virtual void draw(const DrawDesc& desc) = 0;
    virtual void readback(const ReadbackDesc& desc) = 0;
    virtual void endFrame() = 0;
    
    // Resource management
    virtual TextureHandle createTexture(const TextureDesc& desc) = 0;
    virtual BufferHandle createBuffer(const BufferDesc& desc) = 0;
    virtual ShaderHandle createShader(const ShaderDesc& desc) = 0;
    virtual PipelineHandle createPipeline(const PipelineDesc& desc) = 0;
    virtual void destroyTexture(TextureHandle handle) = 0;
    virtual void destroyBuffer(BufferHandle handle) = 0;
};

// Define handle types and descriptor structs
using TextureHandle = uint32_t;
using BufferHandle = uint32_t;
struct TextureDesc { /* width, height, format, usage, etc. */ };
struct BufferDesc { /* size, usage, access, etc. */ };
```

**Accept when:** Project compiles with new headers; no engine integration yet.

#### **Task 2: Implement RhiGL (Desktop OpenGL)**
**Files**: `engine/rhi/RhiGL.h` (new), `engine/rhi/RhiGL.cpp` (new)

**What to do:**
```cpp
class RhiGL : public RHI {
private:
    struct TextureGL { GLuint id; GLenum target; /* ... */ };
    struct BufferGL { GLuint id; GLenum target; /* ... */ };
    std::vector<TextureGL> m_textures;
    std::vector<BufferGL> m_buffers;
    
public:
    bool init(const RhiInit& desc) override {
        // Initialize OpenGL context, check extensions
        return true;
    }
    
    TextureHandle createTexture(const TextureDesc& desc) override {
        GLuint id;
        glGenTextures(1, &id);
        // Configure texture based on desc
        return storeTexture(id, desc);
    }
    
    void draw(const DrawDesc& desc) override {
        // Bind pipeline, resources, issue draw call
    }
    
    // ... implement all RHI methods
};
```

**Accept when:** A minimal test can init RhiGL, clear a render target, and read back pixels.

#### **Task 3: Thread Existing Raster Through RHI**
**Files**: `engine/src/render_system.cpp`, shader loading utilities, mesh upload code

**What to do:**
- Replace direct `glGenTextures`, `glBindTexture`, etc. with `m_rhi->createTexture()`, etc.
- Update shader loading to use `RHI::createShader()`
- Update mesh upload to use `RHI::createBuffer()`
- Keep rendering behavior **identical** to current raster output

**Accept when:** Core sample scenes render with no visual regression (± tiny floating-point noise).

### **Phase 2: Unified Materials (Week 1, Days 4-5)**

#### **Task 4: Introduce MaterialCore and Migrate SceneObject**
**Files**: `engine/include/material_core.h` (new), `engine/include/scene_manager.h`, JSON Ops, loaders

**What to do:**
```cpp
// New unified material
struct MaterialCore {
    glm::vec4 baseColor{1.0f};
    float metallic{0.0f};
    float roughness{0.5f};
    float normalStrength{1.0f};
    glm::vec3 emissive{0.0f};
    float ior{1.5f};
    float transmission{0.0f};
    float thickness{0.0f};
    float attenuationDistance{1.0f};
    float clearcoat{0.0f};
    float clearcoatRoughness{0.0f};
};

// Update SceneObject
struct SceneObject {
    // REMOVE: Material material; (legacy)
    // REMOVE: glm::vec4 baseColorFactor; (PBR)
    // REMOVE: float metallicFactor, roughnessFactor; (PBR)
    
    // ADD: Single source of truth
    MaterialCore material;
    
    // Keep: VAO, transform, name, etc.
};
```

**Update JSON Ops parsing:**
```cpp
// In json_ops.cpp - set_material operation
MaterialCore& mat = targetObj->material;
if (matObj.HasMember("color")) getVec3(matObj["color"], mat.baseColor);
if (matObj.HasMember("metallic")) mat.metallic = matObj["metallic"].GetFloat();
if (matObj.HasMember("roughness")) mat.roughness = matObj["roughness"].GetFloat();
if (matObj.HasMember("ior")) mat.ior = matObj["ior"].GetFloat();
if (matObj.HasMember("transmission")) mat.transmission = matObj["transmission"].GetFloat();
// ... etc for all properties
```

**Accept when:** Raster still renders; JSON Ops still set materials; no visual changes.

#### **Task 5: Adapt Raytracer to MaterialCore**
**Files**: `engine/src/raytracer.cpp`, `engine/src/raytracer_lighting.cpp`

**What to do:**
- Remove PBR→legacy conversion in `loadModel()`
- Update raytracer to read `MaterialCore` directly:

```cpp
// In raytracer.cpp
void Raytracer::loadModel(const ObjLoader& obj, const glm::mat4& M, const MaterialCore& mat) {
    // Direct use - no conversion needed
    for (size_t i = 0; i < Nt; ++i) {
        triangles.emplace_back(v0, v1, v2, mat); // Pass MaterialCore directly
    }
}

// In Triangle constructor
Triangle(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, const MaterialCore& mat)
    : v0(a), v1(b), v2(c), material(mat) {}
```

**Accept when:** Glass/refraction test from `examples/json-ops/glass-sphere-refraction.json --raytrace` looks the same or better.

### **Phase 3: Pass System (Week 2, Days 1-2)**

#### **Task 6: Add Minimal RenderGraph**
**Files**: `engine/include/render_graph.h` (new), `engine/src/render_graph/` (new dir)

**What to do:**
```cpp
struct PassContext {
    RHI* rhi;
    const Scene* scene;
    const Light* lights;
    const CameraState* camera;
    // Shared resources, render targets, etc.
};

class RenderPass {
public:
    virtual ~RenderPass() = default;
    virtual void setup(const PassContext& ctx) = 0;
    virtual void execute(const PassContext& ctx) = 0;
    virtual void teardown(const PassContext& ctx) = 0;
};

class RenderGraph {
private:
    std::vector<std::unique_ptr<RenderPass>> m_passes;
    
public:
    void addPass(std::unique_ptr<RenderPass> pass) {
        m_passes.push_back(std::move(pass));
    }
    
    void execute(const PassContext& ctx) {
        for (auto& pass : m_passes) {
            pass->setup(ctx);
            pass->execute(ctx);
            pass->teardown(ctx);
        }
    }
};

// Implement basic passes:
class GBufferPass : public RenderPass { /* Render geometry to G-buffer */ };
class LightingPass : public RenderPass { /* Deferred lighting */ };
class PostPass : public RenderPass { /* Tone mapping, gamma correction */ };
class ReadbackPass : public RenderPass { /* CPU readback */ };
```

**Accept when:** Raster renders via passes with the same output as before.

#### **Task 7: Wire Material Uniforms (Prep for Transmission)**
**Files**: `engine/shaders/pbr.frag`, uniform buffer object (UBO) layout, material binding code

**What to do:**
```glsl
// In pbr.frag - add new uniforms
uniform float transmission;
uniform float thickness;
uniform float ior;
uniform float attenuationDistance;
uniform float clearcoat;
uniform float clearcoatRoughness;
```

```cpp
// In material binding code
void bindMaterialUniforms(const MaterialCore& mat, Shader* shader) {
    shader->setVec4("baseColorFactor", mat.baseColor);
    shader->setFloat("metallicFactor", mat.metallic);
    shader->setFloat("roughnessFactor", mat.roughness);
    shader->setFloat("ior", mat.ior);
    shader->setFloat("transmission", mat.transmission);
    shader->setFloat("thickness", mat.thickness);
    shader->setFloat("attenuationDistance", mat.attenuationDistance);
    shader->setFloat("clearcoat", mat.clearcoat);
    shader->setFloat("clearcoatRoughness", mat.clearcoatRoughness);
}
```

**Accept when:** No visible change yet; uniforms visible in RenderDoc/debug tools.

### **Phase 4: Screen-Space Refraction (Week 2, Days 3-5)**

#### **Task 8: Implement Screen-Space Refraction (SSR-T)**
**Files**: `engine/shaders/pbr.frag`, shader utilities

**What to do:**
```glsl
// In pbr.frag - simplified SSR-T implementation
vec3 computeSSRRefraction(vec3 viewDir, vec3 normal, float ior, float transmission) {
    if (transmission < 0.01) return vec3(0.0); // Skip if opaque
    
    // Compute refracted direction using Snell's law
    vec3 refractionDir = refract(-viewDir, normal, 1.0 / ior);
    
    // Handle total internal reflection
    if (length(refractionDir) < 0.01) {
        return vec3(0.0); // TIR - no refraction
    }
    
    // Project refracted ray to screen space
    vec3 refractionEnd = vWorldPos + refractionDir * thickness;
    vec4 refractionClip = uProjectionMatrix * uViewMatrix * vec4(refractionEnd, 1.0);
    vec2 refractionUV = (refractionClip.xy / refractionClip.w) * 0.5 + 0.5;
    
    // Sample background color (scene color buffer or environment)
    if (refractionUV.x >= 0.0 && refractionUV.x <= 1.0 && 
        refractionUV.y >= 0.0 && refractionUV.y <= 1.0) {
        return texture(sceneColorTex, refractionUV).rgb;
    } else {
        // Fallback to environment map or background color
        return texture(environmentMap, refractionDir).rgb;
    }
}

// In main():
vec3 refractedColor = computeSSRRefraction(viewDir, normal, ior, transmission);

// Fresnel mixing
float fresnel = fresnelSchlick(max(dot(normal, viewDir), 0.0), 1.0, ior);
vec3 finalColor = mix(refractedColor, reflectedColor, fresnel);
finalColor = mix(baseColor.rgb, finalColor, transmission);
```

**Accept when:** Transmissive materials refract in raster mode; artifacts acceptable; performance reasonable (<16ms).

#### **Task 9: Roughness-Aware Refraction Blur**
**Files**: Same as Task 8

**What to do:**
```glsl
// Approximate micro-roughness by mip bias or small blur
vec3 computeSSRRefractionBlurred(vec3 viewDir, vec3 normal, float ior, float transmission, float roughness) {
    vec3 refractionDir = refract(-viewDir, normal, 1.0 / ior);
    
    // ... same projection logic ...
    
    // Blur based on roughness
    float mipLevel = roughness * 6.0; // Assuming 6 mip levels
    return textureLod(sceneColorTex, refractionUV, mipLevel).rgb;
    
    // Alternative: multi-sample approach for higher quality
    // vec3 blurredColor = vec3(0.0);
    // for (int i = 0; i < 4; ++i) {
    //     vec2 offset = poissonDisk[i] * roughness * 0.01;
    //     blurredColor += texture(sceneColorTex, refractionUV + offset).rgb;
    // }
    // return blurredColor * 0.25;
}
```

**Accept when:** Higher roughness yields softer refraction in raster pipeline.

### **Phase 5: Hybrid Pipeline (Week 3, Days 1-3)**

#### **Task 10: Hybrid Auto Mode + CLI Flag**
**Files**: `engine/src/cli_parser.cpp`, `engine/src/main.cpp`, mode selector utility

**What to do:**
```cpp
// Add CLI flag parsing
enum class RenderMode { Raster, Ray, Auto };

struct CLIOptions {
    RenderMode renderMode = RenderMode::Raster; // Default
    bool isPreview = false; // vs final quality
    // ... existing options
};

// In CLI parser
if (hasFlag("--mode")) {
    std::string mode = getValue("--mode");
    if (mode == "raster") result.options.renderMode = RenderMode::Raster;
    else if (mode == "ray") result.options.renderMode = RenderMode::Ray;
    else if (mode == "auto") result.options.renderMode = RenderMode::Auto;
}

// Mode selection heuristics
class ModeSelector {
public:
    static RenderMode selectMode(const Scene& scene, const CLIOptions& options) {
        if (options.renderMode != RenderMode::Auto) return options.renderMode;
        
        // Check if any material needs raytracing
        for (const auto& obj : scene.getObjects()) {
            const auto& mat = obj.material;
            if (mat.transmission > 0.01f && 
                (mat.thickness > 0.0f || mat.ior > 1.05f)) {
                return options.isPreview ? RenderMode::Raster : RenderMode::Ray;
            }
        }
        return RenderMode::Raster; // Default to faster
    }
};
```

**Accept when:** `--mode auto` behaves as above; `--mode ray|raster` overrides work.

#### **Task 11: Align BSDF Models Between Raster & Ray**
**Files**: `engine/shaders/pbr.frag`, `engine/src/raytracer.cpp`, shared BRDF utilities

**What to do:**
- Ensure both pipelines use identical BRDF models:
  - **GGX distribution** for microfacet normals
  - **Smith masking-shadowing** function  
  - **Schlick Fresnel** approximation
  - **Energy compensation** for multiple scattering

```cpp
// Shared BRDF utilities (can be used by both shader and CPU code)
namespace BRDF {
    inline float DistributionGGX(float NdotH, float roughness) {
        float a = roughness * roughness;
        float a2 = a * a;
        float denom = NdotH * NdotH * (a2 - 1.0f) + 1.0f;
        return a2 / (M_PI * denom * denom);
    }
    
    inline float GeometrySmith(float NdotV, float NdotL, float roughness) {
        float k = (roughness + 1.0f) * (roughness + 1.0f) / 8.0f;
        float ggx1 = NdotV / (NdotV * (1.0f - k) + k);
        float ggx2 = NdotL / (NdotL * (1.0f - k) + k);
        return ggx1 * ggx2;
    }
    
    inline glm::vec3 FresnelSchlick(float cosTheta, const glm::vec3& F0) {
        return F0 + (glm::vec3(1.0f) - F0) * std::pow(1.0f - cosTheta, 5.0f);
    }
}
```

**Accept when:** A metallic/roughness sphere grid closely matches between raster and ray pipelines.

#### **Task 12: Clearcoat & Attenuation Support**
**Files**: Shaders, raytracer volume integration

**What to do:**
```glsl
// In pbr.frag - clearcoat lobe
vec3 computeClearcoat(vec3 normal, vec3 viewDir, vec3 lightDir, float clearcoat, float clearcoatRoughness) {
    if (clearcoat < 0.01) return vec3(0.0);
    
    vec3 H = normalize(viewDir + lightDir);
    float NdotH = max(dot(normal, H), 0.0);
    float VdotH = max(dot(viewDir, H), 0.0);
    
    float D = DistributionGGX(NdotH, clearcoatRoughness);
    float G = GeometrySmith(max(dot(normal, viewDir), 0.0), max(dot(normal, lightDir), 0.0), clearcoatRoughness);
    vec3 F = FresnelSchlick(VdotH, vec3(0.04)); // Fixed F0 for clearcoat
    
    return D * G * F * clearcoat;
}

// Beer-Lambert attenuation for transmission
vec3 applyVolumeAttenuation(vec3 color, float distance, float attenuationDistance, vec3 attenuationColor) {
    if (distance <= 0.0 || attenuationDistance <= 0.0) return color;
    
    float attenuation = exp(-distance / attenuationDistance);
    return mix(attenuationColor, color, attenuation);
}
```

**Accept when:** Clearcoat lobes visible; colored glass attenuates with thickness consistently across pipelines.

### **Phase 6: Production Features (Week 3, Days 4-5 + Week 4, Day 1)**

#### **Task 13: Readback of Auxiliary Attachments**
**Files**: FBO setup, readback utilities, CLI extensions

**What to do:**
```cpp
// Add optional auxiliary outputs
struct ReadbackConfig {
    bool exportDepth = false;
    bool exportNormals = false;
    bool exportInstanceIds = false;
    std::string outputPrefix; // e.g., "render" → "render_depth.png", "render_normals.exr"
};

// CLI: --export depth,normals,ids
// Output: scene.png, scene_depth.png, scene_normals.exr, scene_ids.png
```

**Accept when:** Files are emitted on demand and align spatially with color output.

#### **Task 14: Deterministic Rendering + Metadata**
**Files**: Render entry points, metadata writer

**What to do:**
```cpp
// Accept --seed parameter, propagate to all stochastic code
struct RenderMetadata {
    uint32_t seed;
    std::string engineVersion;
    RenderMode mode;
    float renderTimeMs;
    std::vector<std::string> passNames;
    std::vector<float> passTimesMs;
};

// Write metadata sidecar: render.json
void writeRenderMetadata(const std::string& basePath, const RenderMetadata& meta);
```

**Accept when:** Re-running with same seed yields identical pixels (within floating-point noise); metadata JSON present.

#### **Task 15: Golden Test Suite + Validator CLI**
**Files**: `tests/golden/`, validator implementation

**What to do:**
```bash
# Add canonical test scenes
tests/golden/scenes/
├── glass_spheres.json          # IOR variation (water, glass, diamond)
├── metallic_roughness_grid.json # Material parameter sweep
├── hdri_environment.json       # IBL + environment mapping
├── clearcoat_car_paint.json    # Clearcoat + base layer
└── volume_attenuation.json     # Colored glass thickness test

# Validator CLI
glint validate --suite core --tolerance 0.995  # SSIM threshold
```

**Accept when:** Validator runs locally and reports PASS for current implementation.

### **Phase 7: Platform Support (Week 4, Days 2-3)**

#### **Task 16: WebGL2 Compliance**
**Files**: Shader preprocessing, web build configuration

**What to do:**
- Add precision qualifiers for WebGL2:
```glsl
#ifdef GL_ES
precision highp float;
precision mediump sampler2D;
#endif
```
- Handle NPOT texture limitations
- Ensure MRT (Multiple Render Targets) compatibility
- Add fallback paths for unsupported features

**Accept when:** Web build compiles and runs core scenes with SSR-T or graceful fallback.

#### **Task 17: Performance Profiling Hooks**
**Files**: Timing utilities, profiler integration

**What to do:**
```cpp
class ScopedTimer {
    std::chrono::high_resolution_clock::time_point m_start;
    float& m_output;
public:
    ScopedTimer(float& output) : m_output(output), m_start(std::chrono::high_resolution_clock::now()) {}
    ~ScopedTimer() { 
        auto end = std::chrono::high_resolution_clock::now();
        m_output = std::chrono::duration<float, std::milli>(end - m_start).count();
    }
};

#define PROFILE_SCOPE(name) ScopedTimer timer_##name(passTimings[#name])
```

**Accept when:** Validator outputs include per-pass timing; no measurable performance regression.

#### **Task 18: Error Handling & Graceful Fallbacks**
**Files**: Capability detection, error logging

**What to do:**
```cpp
class RenderCapabilities {
public:
    bool supportsSSR = false;
    bool supportsFloatTextures = false;
    bool supportsMRT = false;
    
    static RenderCapabilities detect(RHI* rhi) {
        RenderCapabilities caps;
        // Query GPU capabilities
        return caps;
    }
};

// Graceful degradation
if (!caps.supportsSSR && material.transmission > 0.01f) {
    Logger::warn("SSR not supported, falling back to alpha blending for transmission");
    // Use alpha blending path
}
```

**Accept when:** No crashes; clear warning logs; sensible degraded output when features unavailable.

### **Phase 8: Cleanup (Week 4, Days 4-5)**

#### **Task 19: Architecture Documentation**
**Files**: `docs/rendering_arch_v1.md`

**What to do:**
- Diagram: Scene → MaterialCore → Mode Selector → RenderGraph → Passes → Readback
- Document RHI interface and backend implementations
- Explain MaterialCore fields and usage
- Describe SSR-T implementation and limitations
- Document auto-mode heuristics and CLI flags

**Accept when:** A new graphics engineer can read the doc and understand the architecture you built.

#### **Task 20: Remove Legacy Material Conversions**
**Files**: Dead code cleanup, include updates

**What to do:**
- Remove old PBR→legacy conversion functions
- Delete unused material structs and adapters
- Update includes and forward declarations
- Clean up any remaining dual-material code paths

**Accept when:** Repo compiles cleanly; validator still passes; no dead code remains.

## 4. Implementation Guidelines

### Development Workflow
1. **Create feature branch** for each task: `git checkout -b task-01-rhi-interface`
2. **Small, focused PRs** - each task should be reviewable in 30-60 minutes
3. **Run validator after each task** to catch regressions early
4. **Update golden images** if intentional changes improve quality
5. **Performance budget**: Keep frame time <16ms for real-time targets

### Testing Strategy
```bash
# After each task, run regression tests
./builds/desktop/cmake/Debug/glint.exe --ops examples/json-ops/sphere_basic.json --render test_raster.png
./builds/desktop/cmake/Debug/glint.exe --ops examples/json-ops/glass-sphere-refraction.json --render test_ray.png --raytrace

# Compare with reference images (manual or automated)
```

### Key Architecture Principles
1. **RHI abstraction**: No direct OpenGL calls outside of RhiGL implementation
2. **Single material truth**: MaterialCore is the only source of material properties
3. **Pass isolation**: Each RenderPass should be self-contained and testable
4. **Graceful degradation**: Always provide fallbacks for unsupported features
5. **Performance awareness**: Profile critical paths and avoid unnecessary allocations

### Common Pitfalls to Avoid
- **Don't break existing CLI behavior** - maintain backward compatibility
- **Don't optimize prematurely** - focus on correctness first, then performance
- **Don't hardcode backend assumptions** - keep RHI interface truly abstract  
- **Don't ignore WebGL constraints** - test on web builds regularly
- **Don't skip validation** - run the golden test suite after significant changes

## 5. Expected Outcomes

### User Experience Improvements
- ✅ Glass materials work identically in both raster and ray modes
- ✅ No need to remember `--raytrace` flag (auto mode handles it)
- ✅ Real-time preview with offline-quality final renders
- ✅ Consistent material behavior eliminates confusion

### Developer Experience Improvements
- ✅ Single MaterialCore struct (no dual storage synchronization)
- ✅ Clean RHI abstraction (easy to add Vulkan/WebGPU backends)
- ✅ Modular pass system (easy to add new effects like SSS, volumetrics)
- ✅ Comprehensive test suite (catch regressions early)

### Performance Characteristics
- ✅ Real-time refraction via SSR-T (<16ms frame budget maintained)
- ✅ GPU backend abstraction with minimal overhead (<5% perf cost)
- ✅ Hybrid mode automatically balances quality vs performance

### Technical Debt Reduction
- ✅ Eliminate material duplication and conversion complexity
- ✅ Replace ad-hoc rendering with structured pass system
- ✅ Remove platform-specific rendering code paths
- ✅ Standardize error handling and capability detection

## 6. Post-Refactor: What You'll Have Built

After completing all 20 tasks, Glint3D will have:

- **Modern Architecture**: RHI abstraction ready for Vulkan/WebGPU
- **Unified Materials**: Single MaterialCore driving both pipelines
- **Hybrid Rendering**: Automatic quality/performance balancing
- **Real-time Refraction**: SSR-T providing fast glass approximation
- **Production Ready**: Deterministic rendering with full metadata
- **Platform Agnostic**: Clean separation between API and implementation
- **Maintainable**: Pass-based system easy to extend and debug

The refactored system will be significantly more maintainable, extensible, and user-friendly while preserving all existing functionality and adding substantial new capabilities.

---

## Advanced Implementation Details

### Enhanced SSR-T (Screen-Space Refraction with Transmission)

#### Practical WebGL2-Compatible Implementation
```glsl
// Enhanced SSR-T for order-independent transparency
vec3 computeSSRTransmission(vec3 viewDir, vec3 normal, float ior, float transmission, 
                           float thickness, float roughness, vec3 attenuationColor) {
    if (transmission < 0.01) return vec3(0.0);
    
    // Depth-aware screen-space refraction with thickness support
    vec3 refractionDir = refract(-viewDir, normal, 1.0/ior);
    vec3 worldRefractEnd = vWorldPos + refractionDir * thickness;
    
    // Project to screen space with proper depth testing
    vec4 clipRefract = uProjectionMatrix * uViewMatrix * vec4(worldRefractEnd, 1.0);
    vec2 refractionUV = (clipRefract.xy / clipRefract.w) * 0.5 + 0.5;
    
    // Weighted Blended OIT for multiple transparent layers
    float sceneDepth = texture(depthBuffer, refractionUV).r;
    float surfaceDepth = gl_FragCoord.z;
    float depthDiff = max(0.0, sceneDepth - surfaceDepth);
    float weight = transmission * (1.0 - exp(-depthDiff * thickness * 10.0));
    
    // Roughness-aware sampling (mip bias or multi-sample blur)
    vec3 refractedColor;
    if (capabilities.supportsMipLevels) {
        float mipLevel = roughness * 6.0;
        refractedColor = textureLod(sceneColorBuffer, refractionUV, mipLevel).rgb;
    } else {
        // Fallback: 4-sample Poisson disk blur
        refractedColor = vec3(0.0);
        const vec2 offsets[4] = vec2[](vec2(-0.7,-0.7), vec2(0.7,-0.7), vec2(-0.7,0.7), vec2(0.7,0.7));
        for (int i = 0; i < 4; ++i) {
            vec2 sampleUV = refractionUV + offsets[i] * roughness * 0.01;
            refractedColor += texture(sceneColorBuffer, sampleUV).rgb;
        }
        refractedColor *= 0.25;
    }
    
    // Beer-Lambert volume attenuation for colored glass
    if (thickness > 0.001) {
        vec3 attenuation = exp(-thickness / attenuationDistance * (vec3(1.0) - attenuationColor));
        refractedColor *= attenuation;
    }
    
    return refractedColor * weight;
}

// Capability detection and graceful fallbacks
bool detectSSRCapabilities() {
    bool hasDepthTexture = checkExtension("GL_OES_depth_texture") || isWebGL2();
    bool hasMipLevels = checkExtension("GL_OES_texture_float_linear");
    bool hasMRT = checkExtension("GL_EXT_draw_buffers") || isWebGL2();
    
    if (!hasDepthTexture) {
        logWarning("Depth textures not supported, SSR-T disabled");
        return false;
    }
    
    return true;
}
```

### AI-Accessible JSON Ops v2

#### Idempotent, Versioned Operations
```cpp
// Enhanced JSON Ops for AI workflows
struct JsonOpsV2 {
    int version = 2;                    // Schema version for compatibility
    uint32_t seed = 0;                  // Deterministic seed for reproducible results
    std::string operationId;            // UUID for deduplication and caching
    std::vector<Operation> operations;  // Idempotent operation sequence
    RenderConfig renderConfig;          // Quality/performance preferences
    
    // Validation and normalization
    bool validate() const;
    void normalize();  // Ensure deterministic operation order
};

// REST API wrapper for batch processing
struct JobRequest {
    JsonOpsV2 operations;
    OutputConfig outputs;
    QualityConfig quality;
    
    // AI-friendly features
    std::string callbackUrl;    // Webhook for completion notification
    int timeoutMs = 300000;     // 5-minute default timeout
    int retryAttempts = 3;      // Automatic retry on failure
};

// Example API usage:
POST /api/v1/jobs
{
    "operations": {
        "version": 2,
        "seed": 42,
        "operationId": "uuid-here",
        "operations": [
            {"op": "load", "path": "models/glass_sphere.obj", "name": "sphere"},
            {"op": "set_material", "target": "sphere", "material": {
                "ior": 1.5, "transmission": 0.9, "thickness": 0.1
            }},
            {"op": "set_camera", "position": [0, 0, 5], "target": [0, 0, 0]},
            {"op": "add_light", "type": "directional", "direction": [-1, -1, -1]}
        ],
        "renderConfig": {
            "mode": "auto",          // Intelligent pipeline selection
            "quality": "preview",    // preview|final
            "width": 512,
            "height": 512
        }
    },
    "outputs": {
        "formats": ["png", "exr"],
        "channels": ["color", "depth", "normals", "ids"],
        "compression": "lz4"
    },
    "callbackUrl": "https://ai-system.example.com/render-complete"
}

Response: { 
    "jobId": "uuid", 
    "status": "queued", 
    "estimatedTimeMs": 15000 
}

// Status polling:
GET /api/v1/jobs/{jobId}/status
{
    "status": "processing",
    "progress": 0.75,
    "currentPass": "ssr_transmission",
    "passProgress": 0.4,
    "elapsedMs": 11200,
    "remainingMs": 3800
}

// Output retrieval:
GET /api/v1/jobs/{jobId}/outputs
{
    "status": "completed",
    "outputs": {
        "color": "https://signed-url/render_color.png",
        "depth": "https://signed-url/render_depth.exr", 
        "normals": "https://signed-url/render_normals.exr",
        "metadata": "https://signed-url/render_metadata.json"
    },
    "metadata": {
        "engineVersion": "0.4.0",
        "renderMode": "hybrid_auto",
        "actualMode": "raster_ssr",
        "seed": 42,
        "totalTimeMs": 14750,
        "passTimings": {
            "gbuffer": 5.2,
            "lighting": 8.7,
            "ssr_transmission": 12.1,
            "post": 3.4,
            "readback": 2.1
        },
        "qualityMetrics": {
            "ssim": 0.987,           // Structural similarity vs golden
            "psnr": 42.3,            // Peak signal-to-noise ratio
            "frameTimeMs": 14.7      // Render performance
        },
        "capabilities": {
            "backend": "OpenGL 3.3",
            "ssrSupported": true,
            "oitSupported": true,
            "embreeEnabled": false,
            "oidnEnabled": false
        }
    }
}
```

### Lightweight Build Matrix

#### Web Preview Build (Optimized for AI)
```cmake
# Minimal WASM build for web deployment
set(GLINT_WEB_PREVIEW ON)
set(GLINT_TARGET_SIZE_MB 2)          # Hard size limit

# Disable heavy features
set(GLINT_ENABLE_EMBREE OFF)         # No CPU ray tracing
set(GLINT_ENABLE_OIDN OFF)           # No AI denoising
set(GLINT_ENABLE_SPIRV OFF)          # No advanced shader pipeline
set(GLINT_ENABLE_ASSIMP OFF)         # Basic OBJ/glTF only

# Enable lightweight features
set(GLINT_ENABLE_SSR_T ON)           # Screen-space refraction
set(GLINT_ENABLE_OIT ON)             # Order-independent transparency
set(GLINT_ENABLE_JSON_OPS_V2 ON)     # Enhanced JSON operations

# Compression and optimization
set(GLINT_COMPRESS_ASSETS ON)        # Brotli-compress embedded assets
set(GLINT_OPTIMIZE_SIZE ON)          # -Os optimization
set(GLINT_STRIP_DEBUG ON)            # Remove debug symbols

# Result: ~2MB WASM + ~500KB assets
```

#### Desktop Final Build (Full Features)
```cmake
# Full-featured desktop build
set(GLINT_DESKTOP_FINAL ON)

# Enable all acceleration
set(GLINT_ENABLE_EMBREE ON)          # Intel Embree ray tracing
set(GLINT_ENABLE_OIDN ON)            # Intel OIDN denoising
set(GLINT_ENABLE_SPIRV ON)           # Future shader pipeline
set(GLINT_ENABLE_ASSIMP ON)          # Full format support

# Production features
set(GLINT_ENABLE_PROFILING ON)       # Performance profiling
set(GLINT_ENABLE_VALIDATION ON)      # Debug validation layers
set(GLINT_ENABLE_TELEMETRY ON)       # Usage metrics

# Future backend preparation
set(GLINT_ENABLE_VULKAN_PREP ON)     # Vulkan headers and validation
set(GLINT_ENABLE_WEBGPU_PREP ON)     # WebGPU headers for future

# Result: ~50MB executable with full capabilities
```

### Quick Milestones (Week-Sized Deliverables)

#### **Week 1: Foundation**
- ✅ RHI (OpenGL backend) + thread existing raster through it
- ✅ MaterialCore unification + raytracer adaptation
- 🎯 **Deliverable**: Current functionality preserved with clean abstraction

#### **Week 2: Core Features**  
- ✅ RenderGraph (minimal pass system)
- ✅ SSR-T implementation + roughness-aware blur
- 🎯 **Deliverable**: Real-time glass materials in raster pipeline

#### **Week 3: Intelligence**
- ✅ Hybrid Auto mode + `--mode raster|ray|auto` CLI
- ✅ JSON Ops v2 + deterministic metadata
- 🎯 **Deliverable**: Intelligent pipeline selection, reproducible renders

#### **Week 4: Production**
- ✅ Validator + golden test suite + CI integration
- ✅ REST /jobs wrapper + signed URL outputs
- 🎯 **Deliverable**: Production-ready AI-accessible rendering service

#### **Optional: Advanced Features**
- ⚡ Intel Embree integration (2-10x faster CPU ray tracing)
- ⚡ Vulkan/WebGPU backend preparation
- ⚡ Advanced volume rendering (subsurface scattering, volumetrics)

Good luck with the refactor! Each milestone builds incrementally toward a modern, AI-friendly, production-ready rendering system that solves the current dual-pipeline limitations while future-proofing for next-generation graphics APIs.