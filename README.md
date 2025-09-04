# Glint3D ✨ — Product Roadmap, Use Cases, and Engineering Plan

A modern **OpenGL viewer and CPU raytracer** with **AI-powered tools** and cross-platform builds.  
Runs on desktop (GLFW/GLAD/ImGui) and web (WebGL2 via Emscripten) with a new React/Tailwind UI.  

> **High‑level pitch:**  
> **Glint3D** is a fast, lightweight 3D viewer and renderer built for **automation and reproducibility**.  
> It runs the same on **desktop and in the browser**, so teams can **view, test, embed, and share** 3D scenes anywhere.  
> Unlike heavy 3D tools, it focuses on **deterministic JSON Ops** and **headless rendering** for CI/CD and bulk rendering.  
> **Optional AI** adds friendly diagnostics and natural‑language scene setup without sacrificing determinism.

---

## Table of Contents
1. [Quick Start](#quick-start)  
2. [New Web UI](#new-web-ui-react--tailwind-in-development)  
3. [JSON Ops v1](#json-ops-v1-bridge)  
4. [Highlights](#highlights)  
5. [New Tools (MVPs)](#new-tools-mvps)  
6. [Controls](#controls)  
7. [Menubar](#menubar)  
8. [Headless + CLI](#headless--scripting-cli)  
9. [Troubleshooting](#troubleshooting)  
10. [Build Notes](#build-notes)  
11. [Repo Layout](#repo-layout)  
12. [Samples](#samples)  
13. [Who It’s For & When It Wins](#who-its-for--when-it-wins)  
14. [Why Use This Viewer](#why-use-this-viewer-strengths)  
15. [Not In Scope](#not-in-scope)  
16. [Roadmap (Phased)](#roadmap-phased)  
17. [Feature Checklist](#feature-checklist-by-phase)  
18. [Use Cases](#use-cases)  
19. [Refactor TODOs](#refactor-todos-code-level)  
20. [Implementation Notes](#implementation-notes)  
21. [Testing Strategy](#testing-strategy)  
22. [Modern Technologies](#modern-technologies-to-consider)  
23. [Milestones](#milestone-checklists)  
24. [Appendix: JSON Ops Example](#appendix-json-ops-example)

---

## Quick Start

### Desktop (Windows)
- Open `engine/Project1.vcxproj` in Visual Studio 2022 (x64).  
- Build and run from the repo root so `shaders/` and `assets/` resolve.  
- Use the top menu to load sample models or recipes. Hold RMB to fly the camera.  

### Desktop (CMake, cross-platform)
```bash
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE=Release
cmake --build builds/desktop/cmake -j
./builds/desktop/cmake/glint
```

### Web (Emscripten)
```bash
emcmake cmake -S . -B builds/web/emscripten -DCMAKE_BUILD_TYPE=Release
cmake --build builds/web/emscripten -j
```
Open `builds/web/emscripten/glint.html` with a local web server. Assets under `engine/assets` and `engine/shaders` are preloaded.  

---

## New Web UI (React + Tailwind) [IN DEVELOPMENT]

Located in `ui/`. This is a modern, product-ready interface that hosts the engine (WASM) in the same page.  

1. Build the web engine (see above). Copy outputs to:
   - `ui/public/engine/glint.js`  
   - `ui/public/engine/glint.wasm`  
   - `ui/public/engine/glint.data` (if generated)  

2. Start the UI:
```bash
cd ui
npm install
npm run dev
```
Open the printed URL. Drag & drop `.obj/.ply/.glb` onto the canvas. Use the Console to run JSON Ops.  

---

## JSON Ops v1 (Bridge)

The engine exposes a simple JSON operation API, used by both the ImGui and Web UIs.

Example:
```json
[
  {"op":"load","path":"assets/models/cube.obj","name":"Hero","transform":{"position":[0,0,-4]}},
  {"op":"add_light","type":"point","position":[3,2,2],"intensity":1.2},
  {"op":"set_camera","position":[0,1,6],"target":[0,1,0],"fov_deg":45}
]
```

Exports to JS (Emscripten):
- `app_apply_ops_json(json)` — apply a single op or array  
- `app_share_link()` — export a shareable state link  
- `app_scene_to_json()` — get current scene snapshot  

Docs: `docs/json_ops_v1.md` (schema in `schemas/json_ops_v1.json`).  

---

## Highlights

- Natural-language commands via AI → JSON Ops v1 scripting  
- Raster path with **PBR (Cook–Torrance)** and flat/Gouraud modes  
- CPU raytracer with BVH + optional OpenImageDenoise  
- **“Why is it black?” diagnostics** with one-click fixes  
- **Performance Coach HUD** with actionable suggestions  
- Shareable scene state (JSON Ops v1) + headless CLI renderer  
- Native OBJ loader; optional unified loader (glTF/GLB/FBX/DAE/PLY) via Assimp  
- Texture cache deduplicates loads; per-object shader choice  

---

## New Tools (MVPs)

### 1) Explain-my-render: “Why is it black?”
Detects and offers one-click fixes:
- Missing normals → Recompute angle-weighted normals  
- Bad winding (mostly backfacing) → Flip triangle order + invert normals  
- No lights / tone-mapped black → Add a neutral key light  
- sRGB mismatch → Toggle framebuffer sRGB  

### 2) Perf Coach HUD
Overlay shows: draw calls, total triangles, materials, textures, VRAM estimate.  
Suggestions:
- Meshes share a material → instancing candidate  
- High draw calls → merge static meshes or instance  
- High triangle count → consider LOD/decimation  
- Optional: **“Ask AI for perf tips”** when AI is enabled  

---

## Controls

- **Camera**: RMB + WASD/E/Q/Space/Ctrl (fly); RMB drag to orbit  
- **Gizmo**: Shift+Q/W/E = Translate/Rotate/Scale; X/Y/Z pick axis  
- **Shortcuts**: L toggles Local/World, N toggles snapping, F11 fullscreen, Delete removes selection  

---

## Menubar

- File → Load Cube/Plane, Copy Share Link, Settings  
- View → Point/Wire/Solid/Raytrace, Fullscreen, Perf HUD  
- Gizmo → Mode, Axis, Local/Snap  
- Samples → Prebuilt recipes (`examples/json-ops/`)  
- Toolbar → Mode, Gizmo, Add Light, Denoise, Why is it black?  

---

## Headless + Scripting (CLI)

Apply JSON Ops v1 and render to PNG without a window:
```bash
./builds/vs/x64/Release/glint.exe --ops examples/json-ops/three-point-lighting.json --render out.png --w 1024 --h 768
```

Options:
- `--ops <file>`: apply JSON Ops v1 from file  
- `--render <out.png>`: render to PNG (optionally with `--w`/`--h`)  
- `--denoise`: enable denoiser (if compiled with OIDN)  

---

## Troubleshooting

- **Black frame (raster)**: use “Why is it black?” panel, add lights, check normals, flip winding if needed.  
- **Dark/washed PBR**: avoid double-gamma (disable FB sRGB if needed).  
- **Headless asset paths**: run from repo root or use absolute paths.  

---

## Build Notes

- Toolchain: Visual Studio 2022, C++17, x64.  
- Third-party: GLFW, ImGui, stb, GLM under `Libraries/`.  
- Optional: Assimp via vcpkg (`USE_ASSIMP`) for non-OBJ formats.  
- Optional: KTX2 via `ENABLE_KTX2` if libktx is available.  

---

## Repo Layout

- `engine/src/` — rendering, UI, loaders, raytracer  
- `engine/include/` — headers  
- `engine/shaders/` — GLSL  
- `engine/assets/�sample models and textures   
- `builds/` — organized build outputs (desktop/web/vs)
- `examples/json-ops/` — sample JSON Operations files
- `docs/` — JSON Ops documentation  

---

## Samples

Use the Samples menu or load recipes manually:
- `examples/json-ops/three-point-lighting.json`  
- `examples/json-ops/isometric-hero.json`  
- `examples/json-ops/studio-turntable.json`
- `examples/json-ops/turntable-cli.json`  

---

## Who It’s For & When It Wins

### Who It’s For
- **Engineering/product teams** needing repeatable viewport renders in CI/CD (game studios, CAD/PLM, digital twins).  
- **Developers & technical artists** embedding a viewer in docs, portals, or internal tools.  
- **ML/vision pipelines** requiring deterministic outputs.  
- **Educators & labs** needing portable viewers with headless capabilities.  

### When It Wins
- Automation pipelines (JSON Ops + headless)  
- Embedding in web/docs with zero install  
- Quick inspection without full DCC complexity  
- Deterministic sharing across teams  

---

## Why Use This Viewer (Strengths)

- Lightweight speed, reproducible workflows, headless rendering  
- Desktop + web parity with embeddable SDK  
- Shareable JSON Ops state and deep links  
- Modern PBR, grid/gizmos, perf HUD  
- Automation-first with optional AI assistants  
- Extensible importers, texture pipeline, and open-source codebase  

---

## Not In Scope
- Not a modeling/animation suite  
- Not a full DCC replacement — complements your pipeline  

---

## Roadmap (Phased)

### Phase 1 – Core Foundations  
- Engine core (OpenGL/WebGL2, raster renderer)  
- CPU raytracer (BVH + optional denoise)  
- JSON Ops v1 (deterministic load, transform, lights, camera, render)  
- Headless CLI rendering  

### Phase 2 – Usability  
- Desktop ImGui UI, gizmos, perf HUD  
- Web React/Tailwind wrapper, drag & drop, JSON console  
- Importer registry (OBJ baseline, Assimp optional)  
- Texture cache + optional KTX2/Basis  

### Phase 3 – Collaboration  
- Web SDK for embedding (`viewer.js`)  
- Shareable links + deep state reproduction  
- Golden-test CI/CD mode  
- Pipeline hooks (GitHub/GitLab templates)  

### Phase 4 – AI Augmentation (Optional)  
- Diagnostics assistant (“Why is it black?”)  
- Natural-language → JSON Ops bridge  
- Perf Coach with AI recommendations  

---

## Feature Checklist by Phase

(See roadmap above for checkboxes and milestones.)

---

## Use Cases

- **Engineering/Dev**: CI/CD golden tests, automated validation, thumbnails  
- **Web/docs**: embeddable viewers, shareable links, reproducible labs  
- **Art/creative**: lookdev previews, perf checks, reference viewer  
- **ML/research**: deterministic datasets, synthetic sweeps, benchmarking  

---

## Refactor TODOs (Code‑level)
(See detailed engineering plan.)

---

## Implementation Notes
(Strict JSON parsing, headless FBO path, optional denoiser, importer fallbacks, AI integration guarded.)

---

## Testing Strategy
- Unit tests for core systems  
- Golden tests for CI (hashes/SSIM)  
- JSON Ops schema validation  
- Importer/web build integration tests  

---

## Modern Technologies to Consider
- ECS (EnTT), render graph, RapidJSON/nlohmann, KTX2/Basis  
- Tooling: CMake + vcpkg, GitHub Actions, Tracy, spdlog, clang-tidy  
- UI/UX: ImGui (desktop), React/TypeScript (web)  

---

## Milestone Checklists

### MVP
- JSON Ops v1 bridge, offscreen FBO render, gizmo rotate/scale  
- Headless CLI end-to-end  
- Cross-platform desktop builds  

### Quality
- Unit + golden tests, clang-tidy, crash handling  

### Features
- IBL/HDR, denoiser, glTF polish + KTX2  

---

## Appendix: JSON Ops Example
```json
[
  {"op":"load","path":"assets/models/cube.obj","name":"Hero","transform":{"position":[0,0,-4]}},
  {"op":"add_light","type":"point","position":[3,2,2],"intensity":1.2},
  {"op":"set_camera","position":[0,1,6],"target":[0,1,0],"fov_deg":45},
  {"op":"render","out":"out/hero.png","width":1280,"height":720}
]
```
-  `schemas/` � JSON schemas (e.g., json_ops_v1.json)   
