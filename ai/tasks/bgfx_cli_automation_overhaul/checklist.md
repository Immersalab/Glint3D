<!-- Machine Summary Block -->
{"file":"ai/tasks/bgfx_cli_automation_overhaul/checklist.md","purpose":"Sequential plan for migrating Glint3D to a BGFX renderer with CLI-focused automation hooks.","exports":["phases","validation_gates"],"depends_on":["task.json","ai/current_index.json"],"notes":["8_phase_migration","cli_first_focus"]}
<!-- Human Summary -->
Checklist orchestrating architecture, integration, automation, validation, and documentation steps needed for the BGFX renderer overhaul.

# BGFX CLI Automation Overhaul Checklist

## Phase 0 – Alignment & Scoping
- [ ] **Capture requirements** – Record renderer, CLI, UI, JsonOps, and automation needs plus success metrics agreed with stakeholders.
- [ ] **Author architecture overview** – Describe BGFX layering, shader pipeline, CLI-first workflow, rollout sequencing, and fallbacks.

## Phase 1 – Build & Dependency Scaffolding
- [ ] **Integrate BGFX deps** – Add bgfx/bx/bimg + shader tools to build (desktop + headless) and document dependency sourcing.
- [ ] **Initialize BGFX platforms** – Hook GLFW handles into bgfx::PlatformData, support headless mode, capture capability matrix.
- [ ] **Establish shader pipeline** – Create deterministic offline compilation flow and store binaries under `resources/shaders/bin`.

## Phase 2 – RHI Abstraction & Resources
- [ ] **Define RHI interfaces** – Buffer/texture/shader/framebuffer APIs plus encoder submission contract for raster + raytrace.
- [ ] **Port asset ownership** – Update SceneManager, TextureCache, Skybox, IBL, and gizmos to create/manage BGFX handles.
- [ ] **Replace RenderSystem GL calls** – Convert draw paths, MSAA resolve, tonemapping, and debug rendering to RHI operations.

## Phase 3 – CLI & Automation Surface
- [ ] **Refactor headless paths** – JsonOps + CLI verbs configure BGFX views deterministically, emit structured logs/metadata.
- [ ] **Expose automation controls** – Extend CLI flags/JSON schema for encoder seeds, backend selection, AI parameter bundles.
- [ ] **Implement deterministic readback** – Use bgfx::readTexture or capture callbacks for renderToPNG and regression tests.

## Phase 4 – UI & Human Workflow
- [ ] **Port ImGui layer** – Adopt bgfx-imgui (or custom) backend, ensuring gizmos/panels draw via RHI.
- [ ] **Unify bootstrap modes** – Ensure CLI/headless skip UI init, desktop mode shares BGFX context safely.
- [ ] **Add status mirroring** – Provide CLI/UI hooks for progress streaming so humans + automation share telemetry.

## Phase 5 – Raytracer & Hybrid Features
- [ ] **Update raytracer blit path** – Upload CPU results via bgfx textures and draw presentation quad without OpenGL.
- [ ] **Validate hybrid subsystems** – Confirm IBL, HDR backgrounds, gradient skybox, gizmos function via BGFX abstractions.
- [ ] **Document determinism** – Ensure RNG seeds and submission order produce repeatable results across pipelines.

## Phase 6 – Validation & Benchmarks
- [ ] **Expand golden suites** – Cover CLI headless, UI interactive, automation-run scenarios with SSIM tracking.
- [ ] **Run build/test matrix** – Windows/Linux/macOS Debug+Release including headless automation cases.
- [ ] **Capture performance metrics** – Benchmark 1080p/4K workloads ensuring <5% regression, document tuning knobs.

## Phase 7 – Documentation & Rollout
- [ ] **Publish artifacts** – Architecture spec, automation contract, UI integration plan, RHI overview, BGFX notes.
- [ ] **Update repo docs** – Refresh CLAUDE.md, docs/architecture, and onboarding materials with new renderer workflow.
- [ ] **Plan rollout** – Update ai/current_index.json, announce migration stages (beta flag, fallback removal), finalize QA.

## Validation Gates
- [ ] **Golden renders** – All reference scenes meet SSIM >= 0.995 compared to BGFX baseline.
- [ ] **Performance budget** – Frame time delta within ±5% vs. legacy OpenGL at 1080p/4K; memory delta <10%.
- [ ] **Deterministic logs** – Repeated headless runs produce identical metadata/log outputs.
- [ ] **Documentation completeness** – Required artifacts exist in `artifacts/` and CLAUDE.md references new architecture.
