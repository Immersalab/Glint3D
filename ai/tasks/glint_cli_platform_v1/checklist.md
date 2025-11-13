<!-- Machine Summary Block -->
{"file":"ai/tasks/glint_cli_platform_v1/checklist.md","purpose":"Tracks work items for delivering Glint CLI platform v1.","exports":[],"depends_on":["ai/tasks/glint_cli_platform_v1/task.json"],"notes":["milestone_a_platform_core"]}
<!-- Human Summary -->
Checklist covering discovery, design, implementation, and validation steps for the CLI platform milestone.

# Glint CLI Platform v1 Checklist

## Phase 1: Foundations & Contracts
- [x] Document CLI product vision, user journeys, and interface contracts (commands, flags, exit codes).
- [x] Finalize project manifest schema (`glint.project.json`) and determinism requirements (run manifest spec).
- [x] Align CLI config precedence with environment + machine-level overrides; capture in design doc.
- [x] Define templating strategy (`glint init --template`) and asset pack handshake.

## Phase 2: Implementation & Tooling
- [x] Implement CLI scaffolding (`glint init`, `validate`, `inspect`, `render`, `config`, `clean`, `doctor`).
- [x] Wire determinism logging: write `renders/<name>/run.json` with complete provenance payload.
- [x] Determinism logging: enforce schema validation + checksum so malformed manifests fail fast.
- [x] Determinism logging: add automated smoke to diff run manifests for identical inputs.
- [x] Add module management (`glint modules list|enable|disable`) and asset synchronization workflow.
- [x] Implement `glint watch` (file graph monitoring) or document gated stub semantics.
- [x] Implement `glint profile` (render perf capture) or document gated stub semantics.
- [x] Implement `glint convert` (asset/material transforms) or document gated stub semantics.
- [x] Ensure commands emit structured output (`--json`) with shared envelope (`status`, `warnings`, `data`).
- [x] Ensure deterministic exit codes for all verbs and document code table in CLI spec.
- [ ] Migrate legacy headless CLI (`--ops`, `CLIParser`) into compatibility shim aligned with new commands.
- [ ] Provide legacy alias coverage tests to guarantee old automation keeps working.
- [x] Move shared logging utilities out of `CLIParser` and unify severity handling across new commands.
- [x] Add logging config (verbosity, color, timestamps) centralized in new logging module.
- [x] Replace legacy help text/docs with the new CLI command reference.

## Phase 3: Validation & Distribution
- [ ] Author command reference + quickstart docs (CLI, templates, determinism, repro).
- [ ] Document project manifest + run manifest schemas inline within CLI docs appendices.
- [ ] Build automated smoke suite covering primary verbs and provenance artifacts.
- [ ] Add negative-path smoke cases (bad config, missing assets, determinism drift) to suite.
- [ ] Capture validation matrix (platforms, module toggles, sample projects).
- [ ] Include reproducibility evidence (hashes, RNG seeds, module revisions) in matrix write-up.
- [ ] Package distribution strategy (installer handoff, PATH integration, shell completion).
- [ ] Publish shell completion scripts (bash/zsh/pwsh) and verify instructions.
- [ ] Update roadmap/communications noting supersedure of legacy CLI installer task.
- [ ] Notify dependent teams (RPC, SDK, Contracts) via internal comms and link to new docs.
