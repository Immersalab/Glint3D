<!-- Machine Summary Block -->
{"file":"ai/tasks/glint_platform_contracts/checklist.md","purpose":"Tracks work for stabilizing Glint data contracts, determinism, and provenance.","exports":[],"depends_on":["ai/tasks/glint_platform_contracts/task.json"],"notes":["schema_versioning","determinism"]}
<!-- Human Summary -->
Checklist to deliver versioned schemas, run manifests, compatibility matrix, and reproducibility guarantees.

# Glint Platform Contracts Checklist

## Phase 1: Contract Definition
- [ ] Finalize schema roadmap (project, scene, run manifest) with versioning strategy.
- [ ] Document determinism requirements (RNG seeds, hashing, asset provenance) and enforcement plan.
- [ ] Define compatibility matrix structure and publishing cadence.
- [ ] Outline deprecation policy and feature gating guidelines.

## Phase 2: Implementation
- [ ] Publish `$schema` documents and validation tooling (CLI + libs) for project/scene/run manifests.
- [ ] Integrate checksum/signature validation for assets, packs, and external references.
- [ ] Implement `glint render --repro <run.json>` flow end-to-end.
- [ ] Wire provenance hash logging across CLI, RPC, SDK surfaces.
- [ ] Add configuration precedence tests and documentation updates.

## Phase 3: Validation & Governance
- [ ] Produce sample manifests (draft/final profiles) and golden repro runs.
- [ ] Create automated regression suite verifying determinism across platforms/devices.
- [ ] Publish compatibility matrix and policy pages in docs.
- [ ] Establish governance workflow (change requests, review board, version bump checklist).
- [ ] Communicate contract availability to partner teams and archive legacy specs.
