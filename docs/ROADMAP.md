# Glint3D — Product Roadmap, Use Cases, and Engineering Plan

> **High‑level pitch:**  
> **Glint3D** is a fast, lightweight 3D viewer and renderer built for **automation and reproducibility**.  
> It runs the same on **desktop and in the browser**, so teams can **view, test, embed, and share** 3D scenes anywhere.  
> Unlike heavy 3D tools, it focuses on **deterministic JSON Ops** and **headless rendering** for CI/CD and bulk rendering.  
> **Optional AI** adds friendly diagnostics and natural‑language scene setup without sacrificing determinism.

---

## Table of Contents
1. [Who It’s For & When It Wins](#who-its-for--when-it-wins)  
2. [Why Use This Viewer (Strengths)](#why-use-this-viewer-strengths)  
3. [Not In Scope](#not-in-scope)  
4. [Name Ideas (Working List)](#name-ideas-working-list)  
5. [Roadmap (Phased)](#roadmap-phased)  
6. [Feature Checklist by Phase](#feature-checklist-by-phase)  
7. [Use Cases](#use-cases)  
8. [Refactor TODOs (Code‑level)](#refactor-todos-codelevel)  
9. [Implementation Notes](#implementation-notes)  
10. [Testing Strategy](#testing-strategy)  
11. [Modern Technologies to Consider](#modern-technologies-to-consider)  
12. [Milestone Checklists](#milestone-checklists)  
13. [Appendix: JSON Ops Example](#appendix-json-ops-example)

---

## Who It’s For & When It Wins

### Who It’s For
- **Engineering/product teams** that need **reliable, repeatable** viewport renders in CI/CD (game studios, CAD/PLM, digital twins).
- **Developers & technical artists** who embed a viewer in docs, web portals, or internal tools.
- **ML/vision pipelines** that require deterministic test scenes and consistent render outputs.
- **Educators & labs** who want a portable viewer with headless capabilities.

### When It Beats Traditional 3D Tools
- **Automation:** Headless + JSON Ops are simpler and more stable for pipelines than scripting full DCC apps.  
- **Embedding:** The same core runs in the browser; easy to present models to users without heavy installs.  
- **Focus:** Quick inspection, basic edits, and renders — without the complexity of modeling/rigging/animation stacks.  
- **Consistency:** Deterministic ops and shareable state reduce “works on my machine” drift across teams.

---

## Why Use This Viewer (Strengths)

- **Lightweight Speed** – Launches fast, loads assets without “projects,” focuses on viewing/inspecting/rendering.  
- **Reproducible Workflows** – Deterministic **JSON Ops** make scenes scriptable & repeatable for CI/docs/pipelines.  
- **Headless Rendering** – Render **PNG** from the command line for batch jobs, regression tests, and server workflows.  
- **Desktop + Web Parity** – Same engine core powers desktop (ImGui) and WebGL build; **embeddable SDK**.  
- **Shareable State** – Encode scene state into a **link** (web) or **JSON** for version control & handoff.  
- **Modern PBR** – Physically‑based shading with sensible **sRGB** handling aligned to glTF conventions.  
- **Practical Editing** – Click‑to‑select, transform gizmo with snapping, grid/axes, basic perf stats.  
- **Automation‑First** – Optional **AI (local Ollama)** assists with ops generation and setup; JSON Ops stays the source of truth.  
- **Extensible by Design** – Importer registry (OBJ, Assimp), texture cache (optional KTX2/Basis), clean core/UI separation.  
- **Open & Inspectable** – Transparent, hackable codebase that can be embedded, adapted, and trusted in production.

---

## Not In Scope
- Not a **modeling/animation** suite (use alongside Blender/Maya/etc.).  
- Not a full‑blown scene editor — by design it’s a **viewer + automation tool** that plays well with your existing toolchain.

---

## Name Ideas (Working List)
*MeshPilot, SceneForge, RenderPilot, RenderCanvas, PhotonForge, LumaMesh, SpectraView, PromptForge, ObjPilot, SceneAtlas*

> Keep the working name **OpenGLOBJViewer** until branding is finalized.

---

## Roadmap (Phased)

### Phase 1 – **Core Foundations (Viewer/Renderer Identity)**
**Goal:** Lightweight, reproducible viewer that works the same on desktop and web.  
- **Engine Core**: OpenGL/WebGL2 raster renderer (PBR + simple fallback).  
- **CPU Raytracer**: BVH acceleration, optional denoise (desktop).  
- **JSON Ops v1**: Deterministic ops (load, transform, add_light, set_camera, render).  
- **Shareable State**: Export/import scene JSON & URL deep links.  
- **Headless Mode**: CLI rendering to PNG with width/height options (CI‑friendly).

### Phase 2 – **Usability Layer**
**Goal:** Make it approachable for developers & technical artists.  
- **Desktop UI (ImGui)**: Menus, perf HUD, scene tree, transform gizmo w/ snapping.  
- **Web Parity (WASM)**: React/Tailwind wrapper, drag & drop, JSON Ops console.  
- **Importer Registry**: Native OBJ; optional Assimp for glTF/FBX/DAE/PLY.  
- **Texture Cache**: Optional KTX2/Basis pipeline.

### Phase 3 – **Collaboration & Embedding**
**Goal:** Drive adoption in pipelines, web portals, and docs.  
- **Embeddable Web SDK**: `viewer.js` API to load/apply Ops/capture PNG.  
- **Shareable Links**: Stable deep links that reproduce scenes.  
- **Versioning & CI/CD**: “Golden test” mode for regression (image hashes).  
- **Pipeline Hooks**: CLI + GitHub Actions/GitLab CI templates.

### Phase 4 – **AI Augmentation (Optional, Last)**
**Goal:** Lower barrier for non‑technical users without breaking determinism.  
- **Diagnostics Assistant**: Plain‑language “Why is it black?” (missing normals/lights/sRGB).  
- **Natural‑Language Ops**: “Add a light above the model” → JSON Ops.  
- **Perf Coach**: “3M triangles; consider LOD/instancing.”

---

## Feature Checklist by Phase

### Phase 1 (Core)
- [ ] Raster renderer (PBR shaders, point/wire/solid modes)  
- [ ] CPU raytracer (BVH; OIDN optional)  
- [ ] JSON Ops v1 (load/transform/add_light/set_camera/render)  
- [ ] Headless CLI rendering (`--ops file.json --render out.png`)  

### Phase 2 (Usability)
- [ ] ImGui desktop UI (menus, gizmos, diagnostics)  
- [ ] React/Tailwind web UI (drag‑drop, JSON console)  
- [ ] Importer registry (OBJ, optional Assimp)  
- [ ] Texture cache + optional KTX2/Basis  

### Phase 3 (Collaboration)
- [ ] Embeddable Web SDK (`viewer.js`)  
- [ ] Shareable deep link state  
- [ ] Golden test mode for CI/CD  
- [ ] CI templates / CLI hooks  

### Phase 4 (AI Augmentation – Optional)
- [ ] AI diagnostics assistant (“Why is it black?”)  
- [ ] NL → JSON Ops translation  
- [ ] Perf Coach recommendations  

---

## Use Cases

### Development & Engineering
- **CI/CD Golden Tests** — Headless renders generate baseline images; compare hashes to flag regressions.  
- **Automated Asset Validation** — Load models via Ops, run diagnostics, export model health reports.  
- **Bulk Thumbnail Generation** — Script consistent PNG previews for entire asset libraries.

### Web & Documentation
- **Embeddable Viewers** — Host the web build in portals or docs; no installs required.  
- **Shareable State Links** — Send a URL that reproduces the same scene setup everywhere.  
- **Interactive Teaching Aids** — Reproducible JSON Ops scenes for labs or coursework.

### Creative & Technical Art
- **Lookdev Previews** — Quick PBR checks before DCC import.  
- **Performance Checks** — Perf HUD for triangles, draw calls, materials to spot bottlenecks.  
- **Reference Viewer** — Transform gizmos and camera presets for clean presentation.

### ML & Research
- **Deterministic Scene Generation** — Produce controlled datasets for training/testing.  
- **Synthetic Data Renders** — Scripted camera sweeps, lighting variations, and outputs.  
- **Benchmarking** — Measure render performance across hardware/driver versions reproducibly.

---

## Refactor TODOs (Code‑level)

- **JSON Ops bridge**
  - Re‑implement JSON Ops v1 against `SceneManager`, `RenderSystem`, `CameraController`, and `Light` (no `Application`).
  - File: `Project1/src/ui_bridge.cpp` (`UIBridge::applyJsonOps`) — currently returns a placeholder error.
- **Headless rendering**
  - Implement offscreen framebuffer path and PNG write.
  - Files: `Project1/src/render_system.cpp` (`renderToTexture`, `renderToPNG`) — both return placeholders.
  - File: `Project1/src/main.cpp` — headless path expects JSON Ops + render; wire to new implementations.
- **CPU raytracer path**
  - `RenderSystem::renderRaytraced` is placeholder (log only); either remove or implement with BVH + output‑to‑screen path.
  - Consider a separate module with golden tests and offscreen output.
- **Denoiser**
  - `RenderSystem::denoise` is stub; integrate Intel Open Image Denoise on desktop behind `OIDN_ENABLED`.
- **PBR pipeline completeness**
  - Tangent space fallback added; ensure tangent buffer generation in `SceneManager` for sources lacking tangents.
  - Validate **sRGB** handling end‑to‑end (textures vs framebuffer) to avoid double gamma.
- **Light & gizmo editing UX**
  - Rotation and scale drag modes not yet wired in the new core; translate works for objects and lights.
  - Snap settings partially exposed; finalize interactions (UI toggles → renderer/core state).
- **AI integration**
  - `NLToJSONBridge` and `AIPlanner` exist (Ollama). Expose endpoint/model settings via UI state to these bridges.
  - Add guardrails, timeouts, and better error surfacing for failed calls.
- **Importers**
  - Assimp/glTF/FBX paths: robust error messages, fallback materials, texture search paths.
  - Optional KTX2/Basis decoding (disabled by default; feature‑gate via CMake).

---

## Implementation Notes

### JSON Ops v1 (Re‑implementation)
Map ops to core systems directly (no `Application`):
- `load` → `SceneManager::loadObject(name, path, pos, scale)` (+ optional rotation)  
- `duplicate` → `SceneManager::duplicateObject(...)` with deltas  
- `remove` → `SceneManager::removeObject(name)` / `Light::removeLightAt(index)`  
- `set_camera` → `CameraController` setters (position/target/front/up/fov/near/far)  
- `select` → scene/light indices/names  
- `render` → `RenderSystem::renderToPNG(path, width, height)`

Use a **strict parser** (RapidJSON) and keep a tolerant mode behind a flag. Validate against `schemas/json_ops_v1.json`.

### Headless Rendering
- Create an FBO with color (RGBA8 sRGB on desktop) + depth; draw scene; readback; flip; write with `stb_image_write`.  
- Handle **MSAA resolve** (blit to non‑MSAA texture).  
- Clamp/gamma‑correct outputs consistently across platforms.

### Denoiser (optional)
- Add `-DOIDN_ENABLED`; detect at runtime and wire a pass with albedo/normal auxiliaries when available.

### Natural Language (optional in Phase 4)
- `AIPlanner` (plan text) + `NLToJSONBridge` (ops JSON) integrated with the UI.  
- Validate outputs against schema before applying; sandbox any disk‑write Ops.

### Importers
- Clear error handling for missing textures/materials; fallback materials.  
- Optional KTX2/Basis for GPU‑friendly textures; gate via `ENABLE_KTX2`.

---

## Testing Strategy

### Unit Tests
- `SceneManager` — transforms, selection, AABB, VAO/EBO lifetimes.  
- `RenderSystem` — viewport/projection math, shader uniform binding, render mode switching.  
- `CameraController` — yaw/pitch vectors, movement/rotation invariants.

### Golden Tests
- Known scenes (cube + lights) rendered headlessly; compare hashes/SSIM.  
- Thresholds allow small driver variance; store baselines per platform in CI artifacts.

### JSON Ops
- Valid/invalid ops; fuzz numeric edge cases and missing fields.  
- Roundtrip: apply ops → snapshot → verify invariants.

### Integration
- Importers with small sample assets; assert triangle counts/materials.  
- Web build sanity: WebGL2 shader compile logs, resource preloading, basic interaction.

---

## Modern Technologies to Consider

### Core & Data
- **ECS (EnTT)** for scalable scene graphs.  
- **Render graph** (FG/graphyte or internal task graph).  
- **RapidJSON** or `nlohmann/json` for robust JSON.  
- **KTX2/Basis** for GPU‑friendly textures.

### Tooling & Quality
- **CMake + vcpkg** for cross‑platform deps (Conan acceptable alternative).  
- **GitHub Actions** for multi‑platform CI.  
- **Tracy/Remotery** for profiling; **spdlog** for logging.  
- **clang‑tidy / clang‑format / include‑what‑you‑use** for code health.

### UI/UX
- **ImGui** (desktop), **React/TypeScript** (web) with a shared JSON Ops bridge.  
- **Embeddable SDK** (`ui/public/engine/objviewer.js`) with a clear API.

---

## Milestone Checklists

### MVP
- [ ] JSON Ops v1 re‑implemented in `UIBridge::applyJsonOps`  
- [ ] `RenderSystem::renderToPNG` offscreen FBO path  
- [ ] Gizmo rotate/scale for objects and lights  
- [ ] Headless CLI: `--ops` + `--render` roundtrip  
- [ ] Windows/macOS/Linux packages

### Quality
- [ ] Unit tests + golden tests in CI  
- [ ] clang‑tidy + format enforcement  
- [ ] Crash handling and opt‑in telemetry

### Features
- [ ] IBL/HDR  
- [ ] Denoiser (OIDN)  
- [ ] glTF2 polish + KTX2

---

## Appendix: JSON Ops Example
```json
[
  {"op":"load","path":"assets/models/cube.obj","name":"Hero","transform":{"position":[0,0,-4]}},
  {"op":"add_light","type":"point","position":[3,2,2],"intensity":1.2},
  {"op":"set_camera","position":[0,1,6],"target":[0,1,0],"fov_deg":45},
  {"op":"render","out":"out/hero.png","width":1280,"height":720}
]
