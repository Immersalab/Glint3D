# Glint3D Web UI - Feature Implementation Status
**Target**: Public Beta (WebGL2 Preview) v0.3.0  
**Status**: In Development  
**Last Updated**: September 2025

## Overview
This document tracks the implementation status of features required for the Public Beta release. The web UI is positioned as a preview build with intentional constraints compared to the desktop version.

**Recommended Status**: Public Beta (WebGL2 Preview)
> Stable enough for demos, share-links, and basic reviews. Not yet feature-parity with desktop; performance and I/O are intentionally constrained.

---

## IMPLEMENTED FEATURES

### Core Architecture
- [x] **React 18 + TypeScript + Tailwind CSS** - Modern frontend stack
- [x] **Vite build system** - Fast development and production builds  
- [x] **Emscripten WASM integration** - C++ engine compiled to WebAssembly
- [x] **JSON Ops v1 Bridge** - Complete API bridge via `app_apply_ops_json()`
- [x] **Tauri desktop wrapper** - Native desktop app capability

### Rendering & Graphics
- [x] **WebGL2 raster viewer** - OpenGL ES 3.0 rendering pipeline
- [x] **PBR shading pipeline** - Metallic-roughness workflow
- [x] **Basic lighting system** - Point, directional, and spot lights
- [x] **Material system** - PBR material support with texture loading
- [x] **Grid and axes rendering** - Scene orientation helpers

### Scene Management  
- [x] **Model loading** - .obj, .ply, .glb, .gltf support via Assimp
- [x] **Scene hierarchy** - Object and light management in Inspector
- [x] **Selection system** - Object and light selection with highlighting
- [x] **Transform operations** - Basic nudge transformations
- [x] **Duplicate/remove** - Scene object manipulation

### Import/Export
- [x] **File drag & drop** - Direct file import via browser APIs
- [x] **Model import** - Multiple format support with file dialog
- [x] **JSON scene export** - Complete scene state serialization
- [x] **JSON scene import** - Scene state restoration from JSON files

### UI Components
- [x] **Canvas host** - WebGL canvas initialization and engine binding
- [x] **Inspector panel** - Scene hierarchy and object/light controls
- [x] **Console panel** - Command input and logging system
- [x] **Top bar** - Navigation, theme toggle, share functionality
- [x] **Command palette** - Keyboard shortcuts and quick actions

### User Experience
- [x] **Theme support** - Dark/light mode with localStorage persistence
- [x] **Real-time updates** - Scene state synchronization (1 second intervals)
- [x] **Error handling** - User-friendly error messages and logging
- [x] **Responsive design** - Basic responsive layout (needs mobile optimization)

---

## PARTIALLY IMPLEMENTED

### State Management & Sharing
- [~] **Share links** - Basic implementation via `app_share_link()` 
  - Function exists and generates links
  - URL-based state restoration (`?state=` parameter handling)
  - Base64URL encoding/decoding for state serialization

### Rendering Features  
- [~] **Tone mapping** - Engine supports it, UI controls missing
  - Engine has Linear/Reinhard/Filmic/ACES support
  - UI sliders for exposure/gamma controls
  - Web UI tone mapping selector

### Asset Pipeline
- [~] **Texture loading** - Basic support, needs optimization
  - Basic texture loading via file system
  - Automatic texture downscaling (4K → 2K)
  - Texture size validation and warnings

---

## NOT IMPLEMENTED (Required for Public Beta)

### Critical Missing Features

#### Performance & Asset Caps
- [ ] **Triangle budget enforcement** - Target ≤1.5M tris, hard cap at 2M
- [ ] **Triangle counter** - Real-time triangle count in UI
- [ ] **Auto-decimation toggle** - Reduce geometry for performance  
- [ ] **Texture size limits** - 4K max with automatic downscaling
- [ ] **Performance warnings** - User notifications for large assets
- [ ] **Memory monitoring** - Browser tab memory usage tracking

#### IBL & Environment Mapping
- [ ] **IBL system** - Image-based lighting with prebaked cubemaps
- [ ] **Environment backgrounds** - Skybox/HDRI environment loading
- [ ] **Reduced resolution IBL** - Web-optimized cubemap sizes (256px)
- [ ] **BRDF LUT** - 512² precomputed lookup table
- [ ] **Environment selection** - Bundled environment switching

#### State Management
- [ ] **URL state encoding** - `?state=` parameter parsing and application
- [ ] **Base64URL serialization** - Compact state encoding for sharing
- [ ] **State validation** - Ensure shared states load correctly
- [ ] **JSON Ops replay** - Full replay system for shared scenes

#### JSON Ops Integration  
- [ ] **Camera presets** - Predefined camera positions and settings
- [ ] **Camera orbit controls** - Smooth orbital camera movement
- [ ] **Background operations** - Set environment/skybox via JSON Ops
- [ ] **Exposure controls** - Tone mapping parameter adjustment

#### Browser Compatibility & Fallbacks
- [ ] **WebGL2 detection** - Graceful fallback for unsupported browsers
- [ ] **Extension detection** - Check for required WebGL extensions
- [ ] **Precision fallbacks** - Float → half-float → UNorm texture paths
- [ ] **sRGB framebuffer handling** - Gamma correction in shader fallbacks
- [ ] **"Reduced quality" badges** - User notification for active fallbacks

#### User Experience & Messaging
- [ ] **Public Beta banner** - Clear status communication to users
- [ ] **Import tooltips** - Guidance for optimal asset formats
- [ ] **CORS error handling** - Clear messages for cross-origin assets
- [ ] **Loading indicators** - Progress feedback for asset loading
- [ ] **Performance mode toggle** - Reduced quality for better performance

#### Asset Format Support
- [ ] **CORS-enabled texture loading** - External texture reference handling  
- [ ] **Same-origin validation** - Security checks for asset references
- [ ] **MTL file parsing** - OBJ material file support with relative textures
- [ ] **Embedded texture priority** - Prefer embedded over external textures

---

## INTENTIONALLY EXCLUDED (Desktop Only)

### High-Performance Features
- [ ] **CPU raytracer** - Headless rendering (desktop only)
- [ ] **Compute shaders** - WebGL2 limitations
- [ ] **OIDN denoising** - CPU-intensive processing
- [ ] **FBX support** - Complex format, desktop only
- [ ] **HDR file uploads** - CORS restrictions, use bundled only

### Advanced Features
- [ ] **AI/NL helpers** - Network features disabled for web
- [ ] **Shadow mapping** - Disabled pending proper implementation
- [ ] **Advanced post-processing** - Performance constraints
- [ ] **Multi-user collaboration** - Network features

---

## ACCEPTANCE CRITERIA FOR PUBLIC BETA

### Core Functionality Tests
- [ ] **Share link test** - `?state=` loads with correct camera/lights/materials
- [ ] **Cross-browser test** - Chrome/Firefox/Safari compatibility  
- [ ] **IBL fallback test** - Precision fallback maintains SSIM ≥ 0.99
- [ ] **Large scene test** - 2.2M triangle scene shows warning, stays stable
- [ ] **Texture limit test** - 8K textures auto-downscale or show friendly error
- [ ] **Tone mapping parity** - Web vs desktop SSIM ≥ 0.990
- [ ] **CORS handling test** - Clear consolidated warnings for failed textures

### Performance Benchmarks
- [ ] **FPS target** - 30-60 FPS on mid-range laptop iGPU at 1080p
- [ ] **Triangle budget** - Smooth interaction with 1M triangles
- [ ] **Memory limits** - Stay within browser tab memory constraints
- [ ] **Load time** - Initial WASM load + first render < 5 seconds

### Browser Support Matrix
- [ ] **Chrome/Edge** - Full feature support (primary target)
- [ ] **Firefox** - Full feature support with minor perf variance
- [ ] **Safari 17+** - Experimental support with precision fallbacks
- [ ] **WebGL2 fallback** - Clear error message if unsupported

---

## IMPLEMENTATION PRIORITY

### Phase 1: Critical Path (Blocking Beta)
1. **State serialization system** - URL-based sharing foundation
2. **IBL pipeline** - Environment mapping and PBR lighting  
3. **Performance caps** - Triangle/texture budgets with warnings
4. **Browser compatibility** - WebGL2 detection and fallbacks

### Phase 2: User Experience  
1. **UI controls** - Tone mapping, exposure, camera presets
2. **Asset validation** - CORS handling, format guidance
3. **Performance feedback** - Real-time counters, quality badges
4. **Loading states** - Progress indicators and error handling

### Phase 3: Polish & Testing
1. **Cross-browser testing** - Safari compatibility, fallback paths
2. **Performance optimization** - Memory usage, rendering efficiency  
3. **Documentation** - User guides, developer docs
4. **Acceptance testing** - Full test suite execution

---

## IMPLEMENTATION ESTIMATE

**Total Features**: 45 required features  
**Implemented**: 25 features (56%)  
**Partially Implemented**: 3 features (7%)  
**Missing**: 17 features (37%)  

**Estimated Work**: 3-4 weeks of focused development  
**Critical Path**: State management + IBL + Performance caps (2 weeks)  
**Polish Phase**: UI/UX improvements (1-2 weeks)

---

## RELATED DOCUMENTATION

- [JSON Ops v1 Specification](../docs/schemas/json_ops_v1.json)
- [Architecture Overview](../CLAUDE.md#architecture-overview)
- [Web Build Instructions](../CLAUDE.md#building-webemscripten)
- [Testing Guidelines](../CLAUDE.md#testing--validation)