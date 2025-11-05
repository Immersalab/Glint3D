<!-- Machine Summary Block -->
{"file":"ai/tasks/glint_cli_installer/checklist.md","purpose":"Tracks phased execution steps for glint CLI + installer initiative.","exports":["phase_steps"],"depends_on":["ai/tasks/glint_cli_installer/task.json"],"notes":["keep_phases_in_order"]}
<!-- Human Summary -->
Checklist sequencing analysis, implementation, packaging, and release validation for the Glint CLI / installer project.

# Glint CLI & Installer Checklist

## Phase 1: Discovery & Specification
- [ ] **Capture headless workflows** - inventory current CLI parser capabilities, headless render flags, and required subcommands.
- [ ] **Define command contract** - nail down `glint render|validate|probe` semantics, structured outputs, exit codes, and config file expectations.
- [ ] **Draft installer scope** - outline supported OS targets, PATH/completion setup, and rollback/uninstall requirements.

## Phase 2: CLI Implementation
- [ ] **Extract headless entrypoint** - create dedicated CLI target linking only engine/core modules and reuse CLI parser safely.
- [ ] **Implement subcommand handlers** - wire render/validate/probe flows with structured logging and error codes.
- [ ] **Document CLI usage** - produce `artifacts/documentation/cli_command_reference.md` with examples and machine-readable output schema.

## Phase 3: Installer Integration
- [ ] **Design installer packaging** - finalize artifact layout, PATH modifications, and optional shell helper placement captured in `installer_distribution_plan.md`.
- [ ] **Automate installer scripts** - add build tooling to emit installer bundles and verify idempotent PATH updates.
- [ ] **Update product docs** - refresh `docs/` content describing install/uninstall and feature parity with GUI flows.

## Phase 4: Validation & Release Prep
- [ ] **Author smoke tests** - script `artifacts/tests/cli_smoke_suite.md` / automation to cover render/validate/probe.
- [ ] **Run headless usage matrix** - capture results in `artifacts/validation/headless_usage_matrix.md` across supported platforms.
- [ ] **Final review & sign-off** - ensure legacy GUI workflows unaffected and installer ready for distribution.

