<!-- Machine Summary Block -->
{"file":"ai/tasks/qem_mesh_simplifier_baseline_plan/checklist.md","purpose":"Tracks repo recon and design deliverables for a baseline QEM mesh simplifier task module.","exports":["phases","validation_gates"],"depends_on":["ai/tasks/qem_mesh_simplifier_baseline_plan/task.json","ai/current_index.json"],"notes":["deliverables_A_to_F","deterministic_qem_requirements","top_priority_preemption"]}
<!-- Human Summary -->
Checklist for producing a repo-grounded QEM simplifier planning package (A-F) with architecture, API, algorithm, test, and build integration guidance.

# Baseline QEM Mesh Simplifier Plan Checklist

## Phase 0 - Re-anchor and Recon Scope
- [x] Re-anchor on `ai/current_index.json`, task files, and active repo state before analysis.
- [x] Capture top-level repository structure and infer build system, module conventions, and viewer/GUI organization.
- [x] Inventory mesh-related code paths and existing mesh data structures or adapters candidates.
- [x] Inventory math dependencies and geometry utility code relevant to QEM implementation.

## Phase 1 - Deliverable A (Repo Recon)
- [x] Produce Repo Recon summary (A) with inferred build system, folder layout, rendering/GUI organization, and conventions.
- [x] List existing mesh representations / adjacency patterns (if present) and math libraries (`glm`, `Eigen`, custom, etc.).
- [x] Call out unknowns explicitly and provide bounded options where repo inspection is inconclusive.

## Phase 2 - Deliverables B and C (Architecture + API)
- [x] Propose removable QEM module directory tree with `core`, `adapters`, `io`, and optional viewer integration layers.
- [x] Define clean public API boundary for `SimplifyOptions`, `SimplifyResult`, and `Simplify(...)` entry points.
- [x] Provide concise header-level usage examples showing glint integration through a thin adapter.
- [x] Document data ownership, lifetime, and mutation rules across core and adapter boundaries.

## Phase 3 - Deliverable D (Baseline Algorithm Plan)
- [x] Specify baseline QEM pipeline: face planes, face quadrics, vertex quadrics, edge costs, collapse loop, and local updates.
- [x] Define required adjacency operations and robust update strategy for edge-collapse bookkeeping.
- [x] Document optimal collapse position solve (3x3 system) with singular fallback policy (endpoints/midpoint).
- [x] Document guards for invalid collapses (degenerate triangles, flips, duplicate vertices, zero-area faces).
- [x] Define deterministic behavior requirements: stable tie-breaks, epsilon policy, and consistent indexing.

## Phase 4 - Deliverables E and F (Implementation + Integration)
- [x] Create staged implementation checklist with checkpoints from adjacency validation to target reduction runs.
- [x] Define non-rendering unit test strategy with known meshes, invariants, and regression cases.
- [x] Describe minimal-churn build integration path for glint (static/object library, include paths, optional feature flags).
- [x] Ensure core library remains free of viewer/GUI/OpenGL/ImGui/Polyscope dependencies.

## Phase 4A - Vertical Slice Scaffold (Early Test Harness)
- [x] Define plugin-ABI-conscious backend contracts and in-process backend interfaces for simplification and render preview.
- [x] Scaffold stubbed `qem_core` headers and pass-through implementation for early integration testing.
- [x] Implement `glint_render_bridge` CLI backend with command generation and process execution.
- [x] Build a minimal external shell UI that exercises simplify and preview backend flows.
- [x] Build the standalone scaffold targets and run a shell smoke test (`status`, `simplify`, `quit`).
- [x] Add dedicated QEM scaffold build/run scripts (`build-and-run.bat`, `build-and-run.sh`) for fast local iteration and smoke runs.
- [x] Keep shell defaults renderer-agnostic (no hardcoded Glint executable or bundled mesh paths); require explicit wiring via backend config/commands.
- [x] Add explicit shell config file workflow (`config load/save/show`) for renderer executable path, example mesh path, and output folder (no auto-discovery).
- [x] Keep preview output and staging folders local to the QEM tool (`tools/qem_simplifier/output`) to reduce cross-project path coupling during early iteration.
- [x] Remove dry-run mode from preview flows to keep the shell/render bridge simple; `preview` and `previewmesh` now execute directly.
- [x] Add preset-aware preview staging in QEM shell (`preview_preset_ops`, `set preset`, auto-append exported camera/light ops during `previewmesh`).
- [x] Add interactive preview tuning workflow: QEM `previewui` launches `glint ui --ops <staged_ops>`, and Glint can export a dedicated preview preset (camera + lights) via menu/console.
- [x] Add OBJ tool-layer mesh import path (tinyobjloader-compatible wrapper) and wire shell `simplify` to use configured `mesh_path` for OBJ input while keeping `qem_core` in-memory only.
- [x] Add `simplifymesh` / `previewsimplified` shell commands to save simplified OBJ output and preview the actual QEM result via the existing Glint CLI render path.
- [x] Add `previewuisimplified` shell command to open the saved simplified OBJ in Glint UI through the staged JsonOps workflow.
- [x] Refactor QEM progress reporting to a structured `qem_core` progress callback/sink with shell-side console formatting (no core stderr logging dependency).
- [x] Simplify shell command surface for iteration: add `clear`, remove standalone `simplify`, and make preview rendering raster-only (no `raytrace` toggle).
- [x] Improve shell `help` output into a clearer sectioned menu since the shell is acting as the temporary external UI.
- [x] Remove low-level raw-ops preview path (`preview` / `set ops`) from the shell UI to keep the preview workflow focused on staged mesh commands only.
- [x] Stylize the `status` command output into a sectioned dashboard format to match the shell's temporary UI presentation.
- [x] Replace verbose QEM progress logs in shell with a single-line progress bar showing only triangle ratio + elapsed time, while keeping final detailed simplify stats.
- [x] Rename the shell workflow commands to shorter, clearer verbs (`simplify`, `render`, `renderqem`, `open`, `openqem`) while keeping legacy aliases temporarily.
- [x] Add compare-view workflow commands (`savecmp`, `rendercmp`, `opencmp`) that stage/load original and simplified meshes side by side via JsonOps.
## Phase 5 - Final Artifact Packaging
- [ ] Produce final A-F deliverable in order with clear headings and concise code snippets only for API/key data structures.
- [ ] Validate output format constraints and ensure assumptions are explicitly labeled.
- [ ] Update progress log and task metadata with completion state/results.

## Validation Gates
- [ ] Repo findings reference actual inspected files and avoid unsupported assumptions.
- [ ] Architecture separates core/adapters/io/viewer cleanly and keeps core system-agnostic.
- [ ] API and algorithm sections document determinism and ownership/lifetime policies explicitly.
- [ ] Build notes align with the build system inferred from repo inspection.





