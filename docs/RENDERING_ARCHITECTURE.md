# Glint3D Rendering Architecture - Quick Reference

**Last Updated**: 2025-01-09  
**Status**: Current as of v0.3.0

## 🚨 Critical Issue: Dual Rendering Architecture

Glint3D has **TWO SEPARATE** rendering pipelines that handle materials completely differently. This frequently causes confusion when implementing glass/transparent materials.

## Pipeline Comparison

| Aspect | **Rasterized** (Default) | **Raytraced** (--raytrace) |
|--------|--------------------------|----------------------------|
| **Activation** | `./glint.exe --render out.png` | `./glint.exe --render out.png --raytrace` |
| **Performance** | ⚡ Real-time (< 1 second) | 🐌 Offline (30+ seconds) |
| **Glass Support** | ❌ **NO REFRACTION** | ✅ **FULL REFRACTION** |
| **Transmission** | ❌ **IGNORED** | ✅ **FULLY SUPPORTED** |
| **Files Used** | `pbr.frag`, `standard.frag` | `raytracer.cpp`, `refraction.cpp` |
| **Material Fields** | `baseColorFactor`, `ior` (F0 only) | `material.ior`, `material.transmission` |

## Common Debugging Scenarios

### "My glass sphere looks like plastic"
```bash
# Problem: Using rasterizer (no refraction)
./glint.exe --ops glass-scene.json --render output.png

# Solution: Add --raytrace flag  
./glint.exe --ops glass-scene.json --render output.png --raytrace
```

### "Transmission parameter has no effect"
The `transmission` parameter is **completely ignored** by the rasterized pipeline. You must use `--raytrace`.

### "IOR only affects edge reflection"
In rasterized mode, IOR only computes F0 = ((n-1)/(n+1))² for basic Fresnel. No ray bending occurs.

## Material Property Routing

```cpp
// JSON Ops input:
{ "ior": 1.5, "transmission": 0.9 }

// SceneObject storage:
├── material.ior = 1.5          ← RAYTRACER uses this (Snell's law)
├── material.transmission = 0.9  ← RAYTRACER uses this (opacity)
├── ior = 1.5                   ← RASTERIZER uses this (F0 only)  
└── baseColorFactor = color     ← RASTERIZER uses this (albedo)
```

## Shader Selection Logic

```cpp
// In RenderSystem::renderObjectsBatched()
bool usePBR = (obj.baseColorTex || obj.mrTex || obj.normalTex);
Shader* shader = usePBR ? m_pbrShader.get() : m_basicShader.get();
```

### Shader Capabilities
- **PBR Shader** (`pbr.frag`):
  - Has: `uniform float ior` (for F0 calculation)
  - Missing: `uniform float transmission` ❌
  - Result: Basic edge Fresnel only, no refraction

- **Basic Shader** (`standard.frag`):
  - Blinn-Phong lighting model
  - No IOR or transmission support

## Refraction Implementation

Located in: `engine/src/refraction.cpp`

### Features
- ✅ Snell's law: `n₁sin(θ₁) = n₂sin(θ₂)`
- ✅ Total Internal Reflection when `sin(θ₂) > 1.0`
- ✅ Fresnel mixing via Schlick's approximation
- ✅ Automatic media transition detection
- ✅ Multi-bounce ray tracing

### Usage
```json
{
  "op": "set_material",
  "target": "GlassSphere",
  "material": {
    "ior": 1.5,           // Glass IOR
    "transmission": 0.9,   // 90% transparent
    "roughness": 0.0      // Smooth surface
  }
}
```

## Debugging Commands

### Verify Raytracer Activation
```bash
./glint.exe --ops scene.json --render test.png --raytrace

# Look for this output:
# [RenderSystem] Screen quad initialized for raytracing
# [RenderSystem] Loading N objects into raytracer  
# [DEBUG] Tracing row 0 of 512...
```

### Material Value Reference
| Material | IOR | Transmission | Notes |
|----------|-----|--------------|--------|
| Air | 1.0 | - | Reference medium |
| Water | 1.33 | 0.85 | Slight blue tint |
| Window Glass | 1.52 | 0.9 | Most common |
| Flint Glass | 1.6 | 0.9 | Higher dispersion |
| Diamond | 2.42 | 0.95 | Maximum IOR |

## File Locations

### Core Architecture
- `engine/include/render_system.h` - Dual pipeline definition
- `engine/src/render_system.cpp` - Pipeline selection logic
- `engine/src/main.cpp` - CLI flag handling (`--raytrace`)

### Rasterized Pipeline
- `engine/shaders/pbr.frag` - PBR shader (limited IOR)
- `engine/shaders/standard.frag` - Basic Blinn-Phong

### Raytraced Pipeline  
- `engine/src/raytracer.cpp` - Main raytracing logic
- `engine/src/refraction.cpp` - Snell's law implementation
- `engine/include/refraction.h` - Refraction utilities

### Material System
- `engine/include/material.h` - Legacy material with transmission
- `engine/include/scene_manager.h` - SceneObject dual storage
- `engine/src/json_ops.cpp` - Material property assignment

## Architecture Status

### Current Limitations
- **Manual Mode Selection**: Users must remember `--raytrace` flag
- **Performance Gap**: 30+ second renders vs real-time
- **Duplicate Material Storage**: Two separate material representations
- **No Hybrid Pipeline**: Can't mix rasterized and raytraced objects

### Planned Improvements
- **Auto-Detection**: Enable raytracing when `transmission > 0`
- **Screen-Space Refraction**: Real-time approximation in rasterized pipeline
- **Material Validation**: Warn about transmission without raytracing
- **Unified Material System**: Single source of truth for properties

---

## Quick Troubleshooting

1. **Glass looks solid?** → Add `--raytrace` flag
2. **Transmission ignored?** → Add `--raytrace` flag  
3. **Only edge Fresnel?** → Add `--raytrace` flag
4. **Renders too slow?** → Consider screen-space approximation
5. **Material not loading?** → Check both `material.*` and direct fields

**Remember**: When in doubt with glass materials, always use `--raytrace`!

---

## Planned Architecture Refactor (v0.4.0)

**Status**: Design complete, implementation planned  
**Documents**: See `docs/REFACTORING_GUIDE.md` for detailed implementation plan

### Problems with Current System
- **Dual Material Storage**: PBR + legacy causing sync issues and conversion drift
- **Manual Mode Selection**: Users must remember `--raytrace` for glass materials
- **Backend Lock-in**: Direct OpenGL calls make Vulkan/WebGPU porting difficult
- **No Real-time Refraction**: Rasterizer cannot approximate glass effects

### Target Solution
- **RHI Abstraction**: Clean separation for future Vulkan/WebGPU backends
- **MaterialCore**: Single unified material struct used by both pipelines
- **Screen-Space Refraction**: Real-time glass approximation in raster mode
- **Hybrid Auto Mode**: Intelligent pipeline selection based on scene content
- **Pass-based Rendering**: Modular system for easy extension

### Migration Timeline
- **20 sequential tasks** over ~3.5 weeks
- **Phase 1**: RHI abstraction and unified materials (1 week)
- **Phase 2**: Pass system and screen-space refraction (1 week)
- **Phase 3**: Hybrid mode and production features (1 week)
- **Phase 4**: Platform support and cleanup (0.5 weeks)

### Expected Benefits
- ✅ Glass materials work in both raster and ray modes
- ✅ No need to remember `--raytrace` flag (auto-detection)
- ✅ Real-time preview + offline-quality final renders
- ✅ Easy to add new backends (Vulkan, WebGPU)
- ✅ Modular pass system for new effects

See `docs/REFACTORING_GUIDE.md` for complete implementation details and task breakdown.