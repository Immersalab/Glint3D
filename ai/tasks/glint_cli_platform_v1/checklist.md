<!-- Machine Summary Block -->
{"file":"ai/tasks/glint_cli_platform_v1/checklist.md","purpose":"Tracks work items for delivering Glint CLI platform v1.","exports":[],"depends_on":["ai/tasks/glint_cli_platform_v1/task.json"],"notes":["milestone_a_platform_core"]}
<!-- Human Summary -->
Checklist covering discovery, design, implementation, and validation steps for the CLI platform milestone.

# Glint CLI Platform v1 Checklist

## Phase 1: Foundations & Contracts
- [ ] Document CLI product vision, user journeys, and interface contracts (commands, flags, exit codes).
- [ ] Finalize project manifest schema (`glint.project.json`) and determinism requirements (run manifest spec).
- [ ] Align CLI config precedence with environment + machine-level overrides; capture in design doc.
- [ ] Define templating strategy (`glint init --template`) and asset pack handshake.

## Phase 2: Implementation & Tooling
- [ ] Implement CLI scaffolding (`glint init`, `validate`, `inspect`, `render`, `config`, `clean`, `doctor`).
- [ ] Wire determinism logging: write `renders/<name>/run.json` with complete provenance payload.
- [ ] Add module management (`glint modules list|enable|disable`) and asset synchronization workflow.
- [ ] Provide watch/profile/convert commands or mark gated for follow-up (with stub behavior).
- [ ] Ensure commands emit structured output (`--json`) and deterministic exit codes for CI.

## Phase 3: Validation & Distribution
- [ ] Author command reference + quickstart docs (CLI, templates, determinism, repro).
- [ ] Build automated smoke suite covering primary verbs and provenance artifacts.
- [ ] Capture validation matrix (platforms, module toggles, sample projects).
- [ ] Package distribution strategy (installer handoff, PATH integration, shell completion).
- [ ] Update roadmap/communications noting supersedure of legacy CLI installer task.
