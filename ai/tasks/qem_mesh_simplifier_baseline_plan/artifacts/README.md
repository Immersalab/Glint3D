<!-- Machine Summary Block -->
{"file":"ai/tasks/qem_mesh_simplifier_baseline_plan/artifacts/README.md","purpose":"Declares planned artifacts for the QEM mesh simplifier repo recon and architecture planning task.","exports":["artifact_manifest"],"depends_on":["ai/tasks/qem_mesh_simplifier_baseline_plan/task.json","ai/tasks/qem_mesh_simplifier_baseline_plan/checklist.md"],"notes":["A_to_F_deliverables","planning_only_no_engine_changes"]}
<!-- Human Summary -->
Artifact manifest for the QEM simplifier planning task. Outputs are documentation-focused and should capture the A-F deliverables requested by the user after repository inspection.

# Artifacts - Baseline QEM Mesh Simplifier Plan

## Planned Outputs

- `artifacts/documentation/qem_baseline_simplifier_plan.md`
  - Primary deliverable containing sections A-F in order.
  - Includes repo recon, removable module architecture, API design, baseline QEM algorithm plan, staged implementation checklist, and build integration notes.

- `artifacts/validation/qem_repo_recon_inventory.md`
  - Optional supporting inventory of inspected files/folders used to ground the A-F report.
  - Useful for auditability when repo conventions or mesh ownership are ambiguous.

## Constraints to Enforce in Artifacts

- Core QEM library must remain independent from viewer/GUI/render backend code.
- Prefer adapting existing glint mesh types via thin adapters instead of rewriting mesh systems.
- Keep I/O optional and separate from algorithm core.
- Document deterministic behavior requirements (tie-breaks, epsilon policy, stable indexing).
- State unknowns and assumptions explicitly when repo inspection is inconclusive.

## Status

Task activated as top priority on `2026-02-24T19:45:03Z`.
All non-completed task modules were placed on hold in `ai/current_index.json` pending completion of this planning task.

## Related Files

- `../task.json`
- `../checklist.md`
- `../progress.ndjson`
- `../../../current_index.json`
