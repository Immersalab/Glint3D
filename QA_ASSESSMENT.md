# GLINT3D QA ASSESSMENT REPORT

Last Updated: 2025-09-06
Assessment Date: 2025-09-05
Version: 0.3.0
Schema/Engine: Schema json_ops_v1.3 | Engine 0.3.0

---

## REQUIREMENTS CHECKLIST

Core Requirements
- [x] PBR raster pipeline (BaseColor, Normal, Metallic/Roughness; sRGB-correct)
- [x] Render modes: point, wireframe, solid, raytrace (CPU)
- [x] Camera: free-fly, orbit, presets (front/back/left/right/top/bottom/isometric)
- [x] Lights: directional, point, spot (intensity/color); simple IBL optional
- [x] Background: solid color, optional HDR skybox/IBL
- [ ] Offscreen render target with MSAA resolve, readback to PNG
- [ ] Deterministic controls: seed, tone mapping (ACES/Reinhard/Filmic/ACES), exposure

---

## IMPLEMENTED FEATURES (SUMMARY)

PBR Raster Pipeline - COMPLETE
- Status: Fully implemented

Render Modes - COMPLETE
- Status: Points, Wireframe, Solid, CPU Raytrace are implemented and switchable.

Camera - COMPLETE
- Status: Free-fly implemented. Presets implemented (front/back/left/right/top/bottom/iso FL/iso BR) with correct orientation; orbit implemented; frame object calculates bounds-based distance.

Lighting - PARTIAL
- Status: Point, Directional, and Spot lights supported. PBR + Standard shaders handle point attenuation, directional lighting (no attenuation), and spot lights with inverse-square attenuation and smooth cone falloff (inner/outer). UI supports add/select/delete, enable/disable, intensity; per-type edits: position (point/spot), direction (directional/spot), and inner/outer cone angles (spot). IBL not implemented.

Background - COMPLETE (HDR/IBL pending)
- Status: Procedural gradient skybox integrated and toggleable; solid color background settable via ops/renderer. HDR/IBL not implemented.

Offscreen - PARTIAL
- Status: Headless render to PNG works. MSAA resolve is not implemented.

Determinism - PARTIAL
- Status: Tone mapping types (linear/reinhard/filmic/aces) + exposure + gamma are surfaced via JSON ops and renderer state; shader uniform plumbing for these is the next step. Seed not yet present.

---

## DETAILED STATUS AND PLANS

1. Camera Presets - HIGH PRIORITY
- Status: Done
- Estimated Effort: 2-3 hours

Requirements (proposal):
```
enum class CameraPreset {
  Front, Back, Left, Right, Top, Bottom,
  IsometricFront, IsometricBack
};
void setCameraPreset(CameraPreset preset, const glm::vec3& target = glm::vec3(0));
```

Plan
- Add preset enum and API to camera_controller.h/.cpp (DONE)
- Compute preset pos/rot around a target with consistent FOV (DONE)
- UI buttons + hotkeys (1-8) and JSON op set_camera_preset (JSON op DONE; UI buttons pending)

Preset Definitions (implemented; target=[0,0,0], up=[0,1,0]; distance chosen to frame bounding sphere using vertical FOV; defaults FOV=45°, margin=25% configurable via shared defaults)
- Front:  pos = [ 0, 0, +R]
- Back:   pos = [ 0, 0, -R]
- Left:   pos = [-R, 0,  0]
- Right:  pos = [+R, 0,  0]
- Top:    pos = [ 0, +R, 0]
- Bottom: pos = [ 0, -R, 0]
- IsoFL:  pos = normalize([-1,  1,  1]) * R
- IsoBR:  pos = normalize([ 1, -1, -1]) * R

2. Advanced Light Types - HIGH PRIORITY
- Status: Not implemented
- Estimated Effort: 4-6 hours

Requirements (proposal):
```
enum class LightType { Point, Directional, Spot };
struct LightSource {
  LightType type;          // default Point
  glm::vec3 position;
  glm::vec3 direction;     // for Dir/Spot
  float spotCutoff;        // inner cone
  float spotOuterCutoff;   // outer cone
  glm::vec3 color;
  float intensity;
  bool enabled;
};
```

Plan
- Extend LightSource and shaders for directional and spot calculations
- UI to choose type and edit direction/cones; indicator rendering update
- JSON ops: add_light accept type, direction, cone params

3. HDR Skybox and IBL - MEDIUM PRIORITY
- Status: Not implemented
- Estimated Effort: 6-8 hours

Plan
- Add HDR loader (.hdr) and convolution shaders
- Generate irradiance/prefilter/BRDF LUT
- PBR integration; UI file picker and intensity control

4. MSAA Offscreen Pipeline - MEDIUM PRIORITY
- Status: Not implemented
- Estimated Effort: 3-4 hours

Plan
- Multisampled FBO and resolve path
- Use in renderToPNG() and expose sample count in UI/CLI

5. Deterministic Rendering Controls - LOW PRIORITY
- Status: Not implemented
- Estimated Effort: 4-5 hours

Requirements (proposal):
```
struct RenderSettings {
  uint32_t seed = 42;
  enum ToneMapping { Linear, Reinhard, ACES, Filmic } tone = Linear;
  float exposure = 1.0f;
  float gamma = 2.2f;
};
```

Plan
- Create RenderSettings and apply in shaders/raytracer
- UI sliders (exposure/gamma) and CLI flags --seed, --tone, --exposure

---

## IMPLEMENTATION HISTORY

2025-09-06 - Asset Root Security Implementation
- Implemented: --asset-root CLI flag for directory traversal protection
- Implemented: PathSecurity namespace with comprehensive validation (path_security.h/cpp)
- Implemented: Path validation integration into JSON ops (load, render_image, set_background)
- Implemented: Comprehensive test suite (tests/path_security_test.cpp, test_path_security.sh)
- Security: Blocks ../../../etc/passwd, ..\\..\\System32, URL-encoded traversals, and paths outside asset root
- Backward Compatible: No restrictions when --asset-root is not specified
- Documentation: Added security guide (docs/ASSET_ROOT_SECURITY.md) and updated help text

2025-09-06 - CLI/Schema/JSON Ops v1.3 Completion
- Implemented: CLI strict schema validation (--strict-schema) with RapidJSON Schema (v1.3 embedded); --schema-version v1.3
- Implemented: Robust exit codes (0=OK, 2=schema, 3=file missing, 4=runtime, 5=unknown flag/invalid arg)
- Implemented: Structured logging with timestamps and levels (--log quiet|warn|info|debug)
- Implemented: JSON Ops v1.3 completion: duplicate, remove/delete alias, orbit_camera, frame_object (bounds-based), select, set_background (color/skybox), exposure, tone_map; preset aliases accept iso-fl/iso-br
- Updated: Docs (docs/json_ops_v1.md) to v1.3 with new ops and render_image
- Improved: frame_object uses world-space AABB to compute center/radius + margin and adjusts camera distance using vertical FOV

2025-09-05 - Camera Presets + Tests
- Implemented: Camera presets in engine with correct orientation and distance framing using vertical FOV and margin
- Implemented: Shared defaults header for preset FOV/margin used by UI, CLI, and tests
- Implemented: JSON op set_camera_preset (front/back/left/right/top/bottom/iso_fl/iso_br)
- Added: Unit tests for determinism and orientation (tests/camera_preset_test.cpp)
- Fixed: Pitch sign when deriving angles from front/target vectors

2025-01-04 - Modern UI and Skybox Implementation
- Added: Modern blue accent colors for UI theme
- Added: Procedural gradient skybox system
- Added: Skybox rendering integration
- Added: Skybox UI controls (View menu + checkbox)
- Reorganized: Moved view controls from settings to menu bar
- Fixed: Visual Studio project includes for skybox files
- Fixed: OpenGL ES shader compatibility for Linux CI

2025-01-03 - Raytracer and UI Improvements
- Fixed: BVH infinite loop in raytracer intersection
- Fixed: OpenMP thread safety issues
- Added: Raytracer command-line integration
- Simplified: UI layout with unified import/export
- Added: JSON scene operations support

---

## COMPLETION METRICS

Method: Priority-weighted (High=3, Medium=2, Low=1)

| Category | Complete | Partial | Missing | % Done |
|----------|----------|---------|---------|--------|
| PBR Pipeline | Yes | - | - | 100% |
| Render Modes | Yes | - | - | 100% |
| Camera System | Free-fly, Presets | Orbit | - | 80% |
| Lighting | Point/Directional/Spot | - | IBL | 80% |
| Background (solid/gradient) | Yes | - | - | 100% |
| HDR/IBL | - | - | All | 0% |
| Offscreen | PNG | - | MSAA Resolve | 70% |
| Deterministic | Tone/Exposure/Gamma | - | Seed | 40% |

Overall Completion: ~72%

---

## NEXT IMPLEMENTATION TARGETS

1. Shader plumbing for tone mapping/exposure/gamma (hook RenderSystem state into shaders)
2. MSAA offscreen pipeline (quality improvement)
3. HDR skybox/IBL (visual enhancement)
4. Seed controls for determinism (--seed)

---

## JSON OPS V1 (CONTRACT)

Core Requirements
- [x] load, duplicate, remove/delete (alias)
- [x] set_camera, set_camera_preset, orbit_camera
- [x] frame_object (bounds-based distance with margin), select
- [x] add_light, set_background (color/skybox toggle), exposure, tone_map
- [x] render_image (out path, width, height)

Implementation Status
- Status: Complete (v1.3)
- Location: engine/src/json_ops.cpp; schema: schemas/json_ops_v1.json (v1.3)
- Notes: Negative examples included under examples/json-ops/negative-tests/*.json; preset aliases accept iso-fl/iso-br

One-Line Semantics (for QA)
- load: add object by path (with optional name, transform).
- duplicate: create a copy of an existing object by name.
- remove: remove object by name (alias delete accepted for backward compatibility).
- set_camera: set camera pose (position + target or front), lens (fov/near/far).
- set_camera_preset: set camera to a named preset around a target and radius.
- orbit_camera: animate or step the camera orbiting a target by yaw/pitch degrees.
- frame_object: position camera to frame object with margin percent and default FOV.
- add_light: add light with type (point/directional/spot), position/direction, color, intensity.
- set_background: set solid color or skybox/HDR reference and intensity.
- exposure: set exposure (and optional gamma) for renderer.
- tone_map: choose tone mapping operator (linear/reinhard/aces/filmic).
- render_image: render current scene to PNG with width/height and samples.

---

## HEADLESS CLI

Implementation Status
- Status: Complete for schema/logging/exit codes/security; core flags stable
- Location: engine/src/main.cpp, engine/src/cli_parser.cpp
- Implemented: --ops, --render, --w, --h, --denoise, --raytrace, --strict-schema, --schema-version v1.3, --log quiet|warn|info|debug, --asset-root; robust exit codes
- Missing (future): --seed <int>, --tone, --exposure, --gamma, --samples <1|2|4|8>, AI/NL helpers

Exit Codes (for CI robustness)
- 0: success
- 2: schema validation error (when --strict-schema)
- 3: operations file not found / I/O
- 4: runtime/render failure
- 5: unknown flag or invalid argument (including missing values)

---

## DESKTOP UI (IMGUI)

Core Requirements
- [ ] Drag-drop models; file menu; recent files
- [ ] Scene tree; selection highlights; transform gizmo (translate/rotate/scale; snap)
- [ ] Camera presets; orbit/fly controls
- [ ] Light editor (type, intensity, color, position/direction)
- [ ] Perf HUD: draw calls, triangles, materials, textures, VRAM estimate

Implementation Status
- Status: Partial
- Location: engine/src/imgui_ui_layer.cpp
- Implemented: File menu (Import/Export), free-fly camera with speed/sensitivity, light list with add/select/delete; per-light enable/disable and intensity; edits for directional (direction), point (position), spot (position/direction/inner/outer cones); directional indicator arrow; settings panel, modern theme, View toggles (Grid/Axes/Skybox), Performance HUD, selection highlight, transform gizmo (translate/rotate/scale rendering and picking via Gizmo)
- Missing: Drag-drop models, recent files, scene tree panel, snap controls, camera preset buttons, more detailed HUD metrics
- Implementation Priority: HIGH (8-12 hours)

---

## REVISED IMPLEMENTATION PRIORITIES

High Priority
1. Shader plumbing for tone mapping/exposure/gamma; CI goldens for these
2. Scene tree and selection panel (foundational UX)

Medium Priority
5. MSAA offscreen pipeline (and sample-count UI)
6. CLI enhancements (--asset-root, --seed, --strict-schema, exit codes, logging)
7. Drag-and-drop model import and recent files
8. Performance HUD details (draw calls, tris, VRAM estimate)

Low Priority
9. HDR skybox/IBL (visual polish)
10. AI/NL integration (optional)
11. Deterministic tone mapping/exposure (after seed support)

---

## ARCHITECTURE OVERVIEW

- Core Rendering: render_system manages raster vs raytrace modes, camera matrices, debug elements, gizmo, and selection highlight.
- Scene: scene_manager stores objects and selection; material.h, pbr_material.h define material properties.
- Camera: camera_controller provides free-fly and look-at controls; presets implemented.
- Lighting: light manages point lights and draws indicators; directional/spot planned.
- Skybox: skybox handles procedural gradient skybox; HDR/IBL planned.
- UI: imgui_ui_layer renders menus, settings, perf HUD, and uses UIBridge to emit commands.
- Bridge: ui_bridge composes Scene/Renderer/Camera/Light, exposes console/history, JSON Ops executor, and share-link export.
- JSON Ops: json_ops parses and applies ops; schema in schemas/json_ops_v1.json.
- CLI Entrypoint: engine/src/main.cpp handles --ops, --render, sizes, denoise, raytrace, help/version.
- Web: WEB_BUILD.md for Emscripten; ui/public/engine/README.md for web bundle expectations.

---

## FEATURE MATRIX (PLATFORMS)

- Desktop (Windows/Linux): Full UI via ImGui, raster + CPU raytrace, headless CLI render to PNG.
- Web (Emscripten/WebGL2): Raster path, share-link replay, no headless; AI/networking disabled.

Known gaps (Web): No compute shaders; memory growth flags enabled; textures prefer PNG/JPG unless KTX2 enabled offline.

---

## DETAILED BACKLOG BY AREA

Rendering Pipeline
- Directional lighting: DONE (shader path, light uniforms, UI controls, ops support).
- Spot lights: DONE (attenuation, smooth cone falloff, UI, ops support).
- Shadows: TODO — add deterministic shadow maps (depth FBO, bias, PCF), gate via `useShadows` uniform; re-enable in shaders and update CI goldens.
- MSAA FBO and resolve: both onscreen and offscreen; sample count in settings and CLI.
- Tone mapping: Reinhard, ACES, Filmic implementations; exposure/gamma controls.
- IBL: irradiance, prefilter, BRDF LUT generation and PBR integration.

Camera and Navigation
- Orbit mode: target-centric orbit with damping and focus/framing.
- Presets: 6 axis + 2 isometric; UI buttons and hotkeys.
- Frame selection: compute tight/loose bounding sphere and move camera.

JSON Ops v1
- Complete ops per schema and proposals: add_light, duplicate, remove (alias of delete), set_camera_preset, orbit_camera, frame_object, select, set_background, exposure, tone_map.
- Strict validation: integrate schemas/json_ops_v1.json with a validator behind --strict-schema.
- Examples: expand examples/json-ops/ with each op demonstrated and golden images for CI.

Headless CLI
- Flags: --strict-schema, --schema-version v1.3, --log quiet|warn|info|debug, --asset-root implemented; --seed pending.
- Exit codes: stable codes for success, validation error, runtime error, file IO error, unknown flag.
- Logging: timestamped, structured messages; quiet mode for CI.

Desktop UI (ImGui)
- Scene tree: hierarchical list, select/delete/duplicate, rename, icons.
- Selection highlights: already present; add material/outline options and thickness control.
- Transform gizmo: snapping (grid/angle), world/local toggle (present), per-axis numeric inputs.
- Drag-drop: from OS onto viewport and Scene tree; accept model and scene JSON.
- Recent files: persisted MRU with thumbnails where available.
- Camera presets: toolbar buttons + hotkeys; status bar readout.
- Performance HUD: show draw calls, tris, textures/materials, VRAM estimates.

Assets and Materials
- Asset root search rules; cache warming; sRGB/linear conversions verified end-to-end.
- Optional KTX2 usage on desktop and web with graceful fallback.

Web
- Embed SDK stabilization: versioning, API docs, and error surfaces.
- Viewer state import/export with ?state= base64 and addressable assets.

---

## ACCEPTANCE CRITERIA (KEY ITEMS)

- Directional light: can be added via UI and JSON ops; shader path produces expected shading on standard PBR test scenes; persists via JSON ops/state share-link; selectable indicator arrow; UI controls to enable/disable and edit direction & intensity. Golden image in CI: PENDING.
- Spot light: can be added via UI and JSON ops; shader path applies inverse-square attenuation and smooth inner/outer cone falloff; UI can edit position, direction, inner/outer cone angles, and intensity. Persisted via JSON ops/state share-link.
- Camera presets: selecting any preset yields the same camera pose for a given target independent of prior state; hotkeys 1-8 map consistently; JSON set_camera_preset works.
- MSAA resolve: --samples=N reduces edge aliasing vs N=1; at N=1 colors bit-match the non-MSAA path.
- Strict schema: invalid ops file fails fast with non-zero exit code and clear message; CI step fails accordingly.
- Asset root security: --asset-root restricts file access to specified directory; blocks ../../../etc/passwd and similar traversal attempts; provides clear error messages; maintains backward compatibility when flag not used. IMPLEMENTED.
- Determinism: same ops + seed produce the same image hash on identical hardware; cross-vendor SSIM >= 0.995 (Web vs Desktop >= 0.990).

---

## QA TEST PLAN

- Unit-adjacent: where feasible, isolate math/utility (e.g., camera preset calculations) and test deterministically.
- Headless E2E: run glint --ops ... --render out.png --w 256 --h 256 for each example; verify output exists and matches schema.
- Visual regression: compare out.png with golden.png using SSIM. Thresholds: default SSIM >= 0.995 or max per-channel delta <= 2 LSB; Web vs Desktop allow SSIM >= 0.990. Upload diff and heatmap artifacts on failure.
- CLI validation: negative tests for unknown flags, bad JSON, missing assets, and schema violations when --strict-schema is on.
- Security testing: verify --asset-root blocks path traversal attempts (../../../etc/passwd, ..\\..\\System32, etc.); run test_path_security.sh and tests/path_security_test.cpp.
- UI smoke: scripted sequences (if available) or manual checklist for menus, toggles, presets, and drag-drop once implemented.

CI Integration (implemented)
- ✅ `validate-golden-images` job in .github/workflows/ci.yml with SSIM comparison and artifact upload
- ✅ Automated golden generation if references missing
- ✅ Python-based comparison with configurable thresholds (desktop: SSIM≥0.995 OR Δ≤2 LSB, web: SSIM≥0.990)
- ✅ Comprehensive test coverage: 5 core scenes covering lighting, camera, tone mapping
- ✅ Failure artifacts: diff images, heatmaps, JSON reports with detailed metrics
- ✅ Workflow dispatch job (regen-goldens-linux) for candidate generation and review

---

## SCHEMA VALIDATION POLICY

- CLI flags: --strict-schema (fail on unknown fields), --schema-version v1.3.
- Canonicalization: when writing ops, produce a canonical JSON (sorted keys, fixed float precision) so diffs are stable and CI reviews are meaningful.
- Validation: schema violations exit with code 2 and a precise error pointer (path to offending field).

---

## SECURITY AND ROBUSTNESS TESTS

- Path traversal: IMPLEMENTED - reject .. that escapes --asset-root; comprehensive validation with tests.
- Oversized textures/OOM: clamp sizes and error cleanly with message; no crash.
- Malformed assets (glTF/OBJ): resilient loaders with actionable errors; fuzz small samples.
- Web: strong CSP, no cross-origin reads without explicit opt-in, no eval in UI.

---

## PACKAGING ACCEPTANCE

- macOS: notarization verified on a clean VM; app launches without quarantine prompts.
- Windows: SmartScreen friendly (signed if possible); no unknown publisher warnings in release pipelines.
- Artifacts include: LICENSE, third-party notices, checksums (SHA256) for binaries.

---

## ROADMAP

- M1 (Core parity): Directional light, camera presets, JSON Ops v1 completion for core ops, strict schema in CLI, baseline golden tests.
- M2 (Quality): MSAA offscreen + UI, orbit/framing, scene tree and selection panel, drag-drop, recent files, expanded HUD.
- M3 (Lighting): Spot lights, HDR skybox loading, IBL stubs and integration, tone mapping/exposure controls and CLI flags.
- M4 (Polish and Web): Embed SDK hardening, asset resolution (--asset-root), deterministic seed in raytracer and raster; packaging scripts.

---

## IMPLEMENTED FEATURES - GOLDEN IMAGE CI

### Golden Image Testing System
**Status**: FULLY IMPLEMENTED ✅  
**Priority**: High (Complete)
**Documentation**: `docs/GOLDEN_IMAGE_CI.md`

**Implementation**: Comprehensive visual regression testing system with:
- **Python SSIM comparison engine** (`tools/golden_image_compare.py`)
- **Automated golden generation** (`tools/generate_goldens.py`) 
- **CI integration** with `validate-golden-images` job
- **Acceptance criteria**: Desktop SSIM ≥0.995 OR channel Δ≤2 LSB; Web vs Desktop SSIM ≥0.990
- **Artifact generation**: Diff images and heatmaps on failure
- **Test coverage**: 5 core scenes (lighting, camera, tone mapping)

**Features**:
- Automatic golden generation if missing
- Batch comparison with detailed metrics
- Visual diff artifacts (side-by-side + enhanced differences)
- SSIM heatmaps for similarity visualization
- JSON reports with comprehensive metrics
- Local testing via `test_golden_system.py`
- Cross-platform threshold handling

**Benefits**:
- Catches visual regressions automatically
- Provides clear failure diagnostics
- Supports intentional golden updates
- Zero false positives with proper thresholds
- Fast CI execution (400x300 test renders)

---

## RISKS AND MITIGATIONS

- Platform drift (GL versions, WebGL2 limits): maintain shader compatibility paths and automated smoke tests on Linux + Emscripten.
- Visual test flakiness: use tolerance windows and seed control; keep reference scenes minimal and deterministic.
- Asset path variability: introduce --asset-root and normalize relative paths in examples; document required assets.
- Performance regressions: track HUD metrics across builds; consider simple timing logs in headless mode.
- **Golden image testing gap**: Manual visual verification required until automated golden image testing is restored.
