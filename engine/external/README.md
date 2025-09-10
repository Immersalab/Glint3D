# Glint3D External Dependencies

This directory contains third-party libraries and tools used by the Glint3D rendering engine. Dependencies are organized by category and usage scope.

## Core Dependencies (Always Required)

### Mathematics & Utilities
- **GLM** - OpenGL Mathematics library for vectors, matrices, transformations
  - Already integrated in `Libraries/include/`
  - Used throughout engine for all math operations

- **fmt** - Fast string formatting library
  - Modern C++ formatting with type safety
  - Replaces legacy printf-style formatting

- **spdlog** - Fast logging library built on fmt
  - Structured logging with multiple output targets
  - Performance-oriented with minimal overhead

### Asset Loading & I/O
- **cgltf** or **tinygltf** - glTF 2.0 loader
  - Lightweight, robust glTF parser
  - Supports PBR materials, animations, extensions
  - Choose cgltf for minimal footprint, tinygltf for full features

- **stb_image** / **stb_image_write** - Image I/O
  - Already integrated for PNG, JPG loading
  - Single-header libraries, battle-tested

- **tinyexr** - OpenEXR image format support
  - HDR/floating-point image loading and saving
  - Essential for environment maps and auxiliary buffers

## Shader Pipeline (Future-Proofing)

### SPIR-V Toolchain
- **shaderc** - GLSL to SPIR-V compiler
  - Compile GLSL shaders to intermediate SPIR-V bytecode
  - Enables cross-platform shader compilation

- **SPIRV-Cross** - SPIR-V cross-compilation
  - SPIR-V → GLSL ES (WebGL2)
  - SPIR-V → MSL (Metal)  
  - SPIR-V → HLSL (DirectX)
  - Future-proofs shader pipeline for all backends

- **SPIRV-Reflect** - SPIR-V reflection
  - Automatic uniform buffer object (UBO) layout detection
  - Binding point inference and validation
  - Reduces manual shader interface maintenance

## Backend Abstraction (Future)

### Vulkan Support
- **volk** - Vulkan function loader
  - Lightweight, header-only Vulkan loader
  - Enables runtime Vulkan API loading

- **VMA (Vulkan Memory Allocator)** - Vulkan memory management
  - AMD's battle-tested memory allocator
  - Handles complex Vulkan memory requirements automatically

### WebGPU Support  
- **Dawn** (Google) or **wgpu-native** (Mozilla) - WebGPU implementation
  - Native WebGPU for desktop applications
  - Same API as browser WebGPU for consistency
  - Choose Dawn for Google ecosystem, wgpu-native for Rust integration

### Multi-Backend Bootstrap (Optional)
- **bgfx** - Multi-backend rendering library
  - **Use case**: Only if you need instant DX/Vulkan/Metal support
  - **Recommendation**: Build custom RHI instead for better control
  - **Trade-off**: Convenience vs flexibility and learning

## Ray Tracing Acceleration (Optional, Desktop Only)

### CPU Ray Tracing
- **Intel Embree** - High-performance ray tracing kernels
  - **Flag**: `GLINT_ENABLE_EMBREE=ON`
  - Optimized BVH construction and traversal
  - SIMD-accelerated intersection routines
  - Significantly faster than custom CPU raytracer

### Denoising
- **Intel Open Image Denoise (OIDN)** - AI-based denoising
  - **Flag**: `GLINT_ENABLE_OIDN=ON`  
  - Already integrated in current codebase
  - Machine learning denoiser for path-traced images
  - Essential for low sample count ray tracing

## Development & Debugging Tools

### Frame Analysis
- **RenderDoc** - Graphics debugging (OpenGL/Vulkan)
  - Frame capture and analysis
  - Shader debugging and profiling
  - Essential for rasterizer development

- **NVIDIA Nsight Graphics** - NVIDIA GPU profiling
  - Advanced GPU performance analysis
  - Shader profiling and optimization

- **AMD Radeon GPU Profiler (RGP)** - AMD GPU profiling
  - Frame timing and GPU utilization analysis

## Build Integration

### CMake Configuration
```cmake
# Core dependencies (always required)
find_package(glm REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)

# Asset loading
option(GLINT_USE_CGLTF "Use cgltf for glTF loading" ON)
option(GLINT_USE_TINYGLTF "Use tinygltf for glTF loading" OFF)

# Shader pipeline (future)
option(GLINT_ENABLE_SPIRV "Enable SPIR-V shader pipeline" OFF)
if(GLINT_ENABLE_SPIRV)
    find_package(shaderc REQUIRED)
    find_package(spirv-cross-core REQUIRED)
    find_package(spirv-cross-glsl REQUIRED)
    find_package(spirv-cross-reflect REQUIRED)
endif()

# Backend support (future)
option(GLINT_ENABLE_VULKAN "Enable Vulkan backend" OFF)
if(GLINT_ENABLE_VULKAN)
    find_package(volk REQUIRED)
    find_package(VulkanMemoryAllocator REQUIRED)
endif()

option(GLINT_ENABLE_WEBGPU "Enable WebGPU backend" OFF)
if(GLINT_ENABLE_WEBGPU)
    find_package(dawn REQUIRED) # or wgpu-native
endif()

# Ray tracing acceleration (optional)
option(GLINT_ENABLE_EMBREE "Enable Intel Embree ray tracing" OFF)
if(GLINT_ENABLE_EMBREE)
    find_package(embree3 REQUIRED)
    target_compile_definitions(${TARGET} PRIVATE GLINT_EMBREE_ENABLED=1)
endif()

# Already integrated
option(GLINT_ENABLE_OIDN "Enable Intel Open Image Denoise" ON)
if(GLINT_ENABLE_OIDN)
    find_package(OpenImageDenoise REQUIRED)
    target_compile_definitions(${TARGET} PRIVATE GLINT_OIDN_ENABLED=1)
endif()
```

### Lightweight Build Strategy

#### Web Preview Build (Minimal)
```cmake
# Optimized for small WASM size
set(GLINT_WEB_PREVIEW ON)
set(GLINT_ENABLE_EMBREE OFF)
set(GLINT_ENABLE_OIDN OFF)
set(GLINT_ENABLE_SPIRV OFF)

# Use stb-only for I/O, basic rasterizer only
# Target: <2MB WASM, raster + SSR-T only
```

#### Desktop Final Build (Full Features)
```cmake
# Full feature set for desktop rendering
set(GLINT_ENABLE_EMBREE ON)
set(GLINT_ENABLE_OIDN ON)
set(GLINT_ENABLE_SPIRV ON)
set(GLINT_ENABLE_VULKAN ON)  # Future

# Includes hybrid ray/raster, advanced denoising, future backends
```

## Package Installation

### vcpkg (Windows, Recommended)
```bash
# Install core dependencies
vcpkg install glm fmt spdlog stb cgltf

# Shader pipeline (future)
vcpkg install shaderc spirv-cross

# Ray tracing (optional)
vcpkg install embree3 openimagedenoise

# Vulkan (future)
vcpkg install volk vulkan-memory-allocator
```

### apt (Ubuntu/Debian)
```bash
# Core dependencies
sudo apt install libglm-dev libfmt-dev libspdlog-dev

# Development tools
sudo apt install libtinyexr-dev

# Ray tracing (from Intel repos)
sudo apt install libembree-dev liboidn-dev

# Vulkan (future)
sudo apt install libvolk-dev
```

### Homebrew (macOS)
```bash
# Core dependencies
brew install glm fmt spdlog

# Development tools
brew install tinyexr

# Ray tracing
brew install embree openimagedenoise
```

## Directory Structure
```
engine/external/
├── README.md                 # This file
├── core/                     # Always-required dependencies
│   ├── fmt/                  # Fast formatting
│   ├── spdlog/              # Logging
│   └── cgltf/               # glTF loading
├── shaders/                  # Shader pipeline (future)
│   ├── shaderc/             # GLSL → SPIR-V
│   ├── spirv-cross/         # SPIR-V cross-compilation
│   └── spirv-reflect/       # SPIR-V reflection
├── backends/                 # Backend APIs (future)
│   ├── vulkan/              # Vulkan: volk, VMA
│   └── webgpu/              # WebGPU: Dawn or wgpu-native
├── raytracing/              # Ray tracing acceleration (optional)
│   ├── embree/              # Intel Embree
│   └── oidn/                # Intel OIDN (already integrated)
└── tools/                   # Development tools
    ├── renderdoc/           # Frame debugging
    └── validation/          # Shader validation tools
```

## Integration Timeline

### Phase 1: Core Dependencies (Week 1)
- Add fmt, spdlog for better logging
- Integrate cgltf for robust glTF loading
- Update stb libraries to latest versions

### Phase 2: Shader Pipeline (Week 2)
- Add shaderc, SPIRV-Cross, SPIRV-Reflect
- Implement GLSL → SPIR-V → target compilation
- Set up CI shader validation

### Phase 3: Optional Acceleration (Week 3)
- Integrate Intel Embree for faster CPU ray tracing
- Add compile-time flags for optional dependencies
- Optimize build configurations (web vs desktop)

### Phase 4: Future Backends (Week 4+)
- Add volk, VMA for Vulkan preparation
- Evaluate Dawn vs wgpu-native for WebGPU
- Design RHI interface with backend flexibility

## Performance Impact

### Core Dependencies
- **fmt**: Negligible overhead, faster than iostreams
- **spdlog**: <1% CPU overhead for logging
- **cgltf**: Small memory footprint, faster than Assimp for glTF

### Optional Dependencies
- **Embree**: 2-10x faster ray intersection vs custom BVH
- **OIDN**: GPU/CPU denoising, significant quality improvement
- **SPIR-V pipeline**: Minimal runtime cost, better development experience

### Binary Size Impact
- **Web build (minimal)**: ~2MB WASM with compression
- **Desktop build (full)**: ~50MB with all optional features
- **Mobile build (future)**: ~10MB with selective features

## Compatibility Matrix

| Dependency | Windows | macOS | Linux | Web | Mobile |
|------------|---------|-------|-------|-----|--------|
| fmt        | ✅      | ✅     | ✅     | ✅   | ✅      |
| spdlog     | ✅      | ✅     | ✅     | ✅   | ✅      |
| cgltf      | ✅      | ✅     | ✅     | ✅   | ✅      |
| tinyexr    | ✅      | ✅     | ✅     | ✅   | ✅      |
| shaderc    | ✅      | ✅     | ✅     | ✅   | ✅      |
| SPIRV-Cross| ✅      | ✅     | ✅     | ✅   | ✅      |
| Embree     | ✅      | ✅     | ✅     | ❌   | ❌      |
| OIDN       | ✅      | ✅     | ✅     | ❌   | ❌      |
| Vulkan     | ✅      | ✅     | ✅     | ❌   | ✅      |
| WebGPU     | ✅      | ✅     | ✅     | ✅   | ✅      |

## License Compatibility

All selected dependencies use permissive licenses compatible with commercial use:

- **MIT**: fmt, spdlog, cgltf, tinyexr, volk
- **Apache 2.0**: Embree, OIDN, shaderc, SPIRV-Cross
- **BSD-style**: stb libraries, GLM, VMA
- **ISC**: Dawn (permissive)

No GPL or copyleft dependencies to ensure commercial deployment flexibility.