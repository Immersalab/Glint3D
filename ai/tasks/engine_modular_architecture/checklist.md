# Engine Modular Architecture Checklist

## Phase 1: Discovery & Blueprint ✅ (COMPLETED)
- [x] **Inventory current layout** — Document how `engine/src`, `engine/include`, `engine/Libraries`, `engine/external`, and `renders/` are used today and note tight couplings.
- [x] **Draft migration blueprint** — Produce `artifacts/documentation/architecture_migration_plan.md` outlining incremental moves, temporary bridges, and rollback paths.
- [x] **Review dependency ownership** — Confirm which third-party libraries stay vendored versus managed externally and capture in `directory_mapping_matrix.md`.
- [x] **Design output directory structure** — Plan `output/{renders,exports,cache}` hierarchy to organize generated artifacts and clarify separation from source/resources.

## Phase 2: Introduce Modular Scaffolding
- [ ] **Create new directory skeleton** — Add `engine/core`, `engine/modules`, `engine/platform`, and `third_party/{vendored,managed}` without relocating code yet.
- [ ] **Establish output directory structure** — Create `output/renders/`, `output/exports/`, `output/cache/` with README.md and .gitkeep placeholders; update .gitignore to exclude `output/` while preserving structure files.
- [ ] **Wire build aliases** — Update CMake to allow both old and new paths simultaneously via clearly marked temporary switches (with TODOs to remove).
- [ ] **Smoke-test builds** — Verify desktop and web builds succeed using the transitional layout before moving files.

## Phase 3: Migrate in Batches
- [ ] **Move core engine sources** — Relocate foundational systems into `engine/core` and adjust includes; eliminate transitional aliases for moved files.
- [ ] **Isolate optional modules** — Shift ray tracing, gizmo, and other optional features into `engine/modules`, guarding them with build options.
- [ ] **Consolidate third-party code** — Move vendored libs into `third_party/vendored` and purge `engine/Libraries` references.
- [ ] **Migrate renders directory** — Move existing `renders/` content to `output/renders/`, update any hardcoded paths in code/docs/tests, and remove old `renders/` directory.

## Phase 4: Harden & Clean Up
- [ ] **Retire temporary switches** — Remove any bridging CMake options or compatibility wrappers introduced during migration.
- [ ] **Update documentation** — Refresh README, developer docs, and onboarding material to reflect the new structure.
- [ ] **Validate parity** — Capture results in `artifacts/validation/build_matrix.md` proving builds pass and no dead/legacy code lingers.
