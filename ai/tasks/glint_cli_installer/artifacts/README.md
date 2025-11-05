<!-- Machine Summary Block -->
{"file":"ai/tasks/glint_cli_installer/artifacts/README.md","purpose":"Explains artifact expectations for the Glint CLI installer task.","exports":["artifact_overview"],"depends_on":["ai/tasks/glint_cli_installer/task.json"],"notes":["update_when_artifacts_change"]}
<!-- Human Summary -->
Reference for where CLI + installer deliverables, docs, and validation evidence live for this task module.

# Glint CLI Installer Artifacts

| Path | Purpose |
|------|---------|
| `documentation/cli_command_reference.md` | Command synopsis, flags, examples, structured output contract. |
| `documentation/installer_distribution_plan.md` | Packaging blueprint covering supported OSes, PATH logic, uninstall steps. |
| `code/glint_cli_entrypoint_design.md` | Architectural notes / patches for the CLI target and headless linkage. |
| `tests/cli_smoke_suite.md` | Test plan or scripts for CLI subcommand smoke coverage. |
| `validation/headless_usage_matrix.md` | Captured execution matrix of CLI runs across environments/platforms. |

Populate each file deterministically; avoid placeholders. Remove `.gitkeep` once artifacts are authored.
