# Glint3D ✨ Roadmap, Use Cases, and Engineering Plan

A modern **lightweight 3D rendering engine** with **automation and AI-supported workflows**.  
Runs on **desktop (OpenGL/GLFW/ImGui)** and **web (WebGL2 via Emscripten)**, with a new **React/Tailwind UI** in development.  

> **High-Level Pitch:**  
> **Glint3D** is a fast, automation-first 3D viewer and renderer built for **determinism, reproducibility, and AI integration**.  
> It runs the same on **desktop and in the browser**, so teams can **view, embed, test, and share** 3D scenes anywhere.  
> Unlike heavy DCC tools, Glint3D focuses on **JSON Ops scripting**, **headless rendering**, and **shareable results**.  
> **AI support** means external AI systems can easily drive rendering pipelines, generate datasets, or analyze outputs.

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
14. [Why Use Glint3D](#why-use-glint3d-strengths)  
15. [Not In Scope](#not-in-scope)  
16. [Graphics API Evolution Roadmap](#graphics-api-evolution-roadmap)  
17. [Roadmap (Phased)](#roadmap-phased)  
18. [Feature Checklist](#feature-checklist-by-phase)  
19. [Use Cases](#use-cases)  
20. [Refactor TODOs](#refactor-todos-code-level)  
21. [Implementation Notes](#implementation-notes)  
22. [Testing Strategy](#testing-strategy)  
23. [Modern Technologies](#modern-technologies-to-consider)  
24. [Milestones](#milestone-checklists)  
25. [Appendix: JSON Ops Example](#appendix-json-ops-example)

---

## Quick Start

### Desktop (Windows)
- Generate project files: `cmake -S . -B builds/desktop/cmake`  
- Open `builds/desktop/cmake/glint.sln` in Visual Studio 2022 (x64).  
- Build and run from the repo root so `shaders/` and `assets/` resolve.  
- Use the top menu to import assets or apply JSON recipes. Hold RMB to fly the camera.  

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

Glint3D’s JSON Ops API is **the automation core**. It’s designed for both **human reproducibility** and **AI-driven pipelines**, ensuring renders are deterministic and scriptable.  

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

- **Automation-first**: JSON Ops for repeatable, scriptable scene setup and rendering  
- **Headless rendering + CLI** for CI/CD, bulk jobs, and dataset generation  
- **AI-supported**: external AI agents can issue Ops, request batches, or validate outputs  
- **Cross-platform parity**: same engine on **desktop + web**, no GPU installs required  
- **Two rendering paths**:  
  - **Raster (real-time)** with modern PBR (Cook–Torrance), gizmos, and SSR transparency  
  - **CPU raytracer** with BVH acceleration and optional denoising  
- **Shareable states**: export Ops, scene JSON, and deep links  
- **Diagnostics tools**: *“Why is it black?”* checks, performance HUD, actionable tips  

---

## New Tools (MVPs)

### 1) Explain-my-render
Quick, automated diagnostics with **one-click fixes** for common issues:  
- Missing normals → Recompute  
- Bad winding → Flip and invert  
- No lights → Add neutral key light  
- sRGB mismatch → Toggle framebuffer sRGB  

### 2) Perf Coach HUD
Overlay with actionable insights (draw calls, VRAM, instancing candidates) — usable by humans or **AI pipelines**.  

---

## Controls

- **Camera**: RMB + WASD/E/Q/Space/Ctrl (fly); RMB drag to orbit  
- **Gizmo**: Shift+Q/W/E = Translate/Rotate/Scale; X/Y/Z pick axis  
- **Shortcuts**: L toggles Local/World, N toggles snapping, F11 fullscreen, Delete removes selection  

---

## Menubar

- File → Load, Copy Share Link, Settings  
- View → Point/Wire/Solid/Raytrace, Fullscreen, Perf HUD  
- Gizmo → Mode, Axis, Local/Snap  
- Samples → Prebuilt recipes (`examples/json-ops/`)  
- Toolbar → Mode, Gizmo, Add Light, Denoise, Diagnostics  

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

- **Black frame**: run diagnostics (*Why is it black?*), add lights, check normals  
- **Dark/washed PBR**: avoid double-gamma (disable FB sRGB if needed)  
- **Headless asset paths**: run from repo root or use absolute paths  

---

## Build Notes

- Toolchain: Visual Studio 2022, C++17, x64  
- Third-party: GLFW, ImGui, stb, GLM  
- Optional: Assimp (multi-format import), libktx (KTX2 textures)  

---

## Repo Layout

- `engine/src/` — rendering, UI, loaders, raytracer  
- `engine/include/` — headers  
- `engine/shaders/` — GLSL  
- `engine/assets/` — sample models/textures   
- `builds/` — organized build outputs (desktop/web/vs)  
- `examples/json-ops/` — sample JSON Ops  
- `docs/` — JSON Ops documentation  

---

## Samples

- `examples/json-ops/three-point-lighting.json`  
- `examples/json-ops/isometric-hero.json`  
- `examples/json-ops/studio-turntable.json`  
- `examples/json-ops/turntable-cli.json`  

---

## Who It’s For & When It Wins

### Who It’s For
- **Engineering/product teams** needing repeatable viewport renders in CI/CD  
- **Developers/technical artists** embedding lightweight viewers in docs or portals  
- **AI & ML teams** needing deterministic datasets or synthetic sweeps  
- **Educators & labs** needing portable, zero-install 3D visualization  

### When It Wins
- Automated pipelines (JSON Ops + headless CLI)  
- Cross-platform reproducibility (desktop ↔ web)  
- Shareable links with zero install  
- **AI-supported workflows** that demand scalable, scriptable rendering  

---

## Why Use Glint3D (Strengths)

- **AI-supported automation**: easy integration with pipelines, agents, or CI/CD  
- **Lightweight & reproducible**: no heavy DCC installs or GPU dependency  
- **Web + desktop parity**: same results everywhere  
- **Instant sharing**: JSON Ops snapshots + deep links  
- **Modern rendering**: PBR materials, diagnostics, perf HUD  

---

## Not In Scope
- Not a modeling/animation suite  
- Not a full DCC replacement — complements existing pipelines  

---

## Graphics API Evolution Roadmap

### Current: OpenGL/WebGL2 Foundation
- **Desktop**: OpenGL 3.3+  
- **Web**: WebGL 2.0  
- **Status**: Stable cross-platform baseline  

### Next: Vulkan/WebGPU Migration (Planned)
- **Desktop Vulkan**: modern low-level API  
- **Web WebGPU**: next-gen graphics with compute  
- **Benefits**: explicit resource management, multi-threading, compute integration  

### Future: Advanced Rendering
- **Gaussian Splatting**: photorealistic point-based rendering  
- **Neural Radiance Fields (NeRF)**: AI-supported view synthesis  
- **Hybrid Pipelines**: raster + neural methods  

---

## Roadmap (Phased)

### Phase 1 – Core Foundations  
- Raster renderer (OpenGL/WebGL2)  
- CPU raytracer (BVH + optional denoise)  
- JSON Ops v1  
- Headless CLI rendering  

### Phase 2 – Usability  
- Desktop ImGui UI, gizmos, perf HUD  
- Web React/Tailwind wrapper  
- Importer registry (OBJ baseline, Assimp optional)  
- Texture cache + KTX2/Basis support  

### Phase 3 – Collaboration  
- Web SDK for embedding  
- Shareable links + deep state  
- Golden-test CI/CD mode  
- Pipeline hooks (CI templates)  

### Phase 4 – AI Support  
- Diagnostics assistant  
- Natural-language → JSON Ops bridge  
- Perf Coach with AI-backed recommendations  

### Phase 5 – Graphics Evolution  
- Vulkan/WebGPU backends  
- Neural rendering (splats/NeRF)  
- Advanced hybrid pipelines  

---

## Feature Checklist by Phase
*(matches roadmap above)*

---

## Use Cases

- **Engineering/Dev**: CI/CD golden tests, automated validation, thumbnails  
- **Web/docs**: embeddable viewers, reproducible labs, shareable states  
- **Creative/lookdev**: quick previews, perf checks, lightweight viewer  
- **ML/research**: deterministic datasets, sweeps, benchmarking  

---

## Refactor TODOs (Code-level)
*(see engineering notes; focus on RHI abstraction, unified MaterialCore, SSR refraction, hybrid mode)*

---

## Implementation Notes
- Strict JSON parsing  
- Headless FBO path  
- Optional denoiser  
- Importer fallbacks  
- AI support guarded by build flags  

---

## Testing Strategy

```bash
# Run all test categories
tests/scripts/run_all_tests.sh
```

Categories:
- **Unit tests** (`tests/unit/`)  
- **Integration tests** (`tests/integration/`)  
- **Security tests** (`tests/security/`)  
- **Golden image tests** (`tests/golden/`) with SSIM/PSNR  
- **Test assets** (`tests/data/`)  

---

## Modern Technologies to Consider
- ECS (EnTT), lightweight RenderGraph  
- RapidJSON / nlohmann JSON  
- Tracy profiler, spdlog  
- clang-tidy, GitHub Actions CI  
- ImGui (desktop), React/TypeScript (web)  

---

## Milestone Checklists

### MVP
- JSON Ops v1 bridge  
- Offscreen FBO render  
- Headless CLI  
- Cross-platform desktop builds  

### Quality
- Unit + golden tests  
- clang-tidy, crash handling  

### Features
- IBL/HDR  
- Denoiser integration  
- glTF/KTX2 polish  

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
- `schemas/` — JSON schemas (e.g., json_ops_v1.json)  
