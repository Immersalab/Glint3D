# Engine Modular Architecture Task Artifacts

This directory captures deliverables generated while modularizing the engine layout.

- `documentation/architecture_migration_plan.md` — Detailed blueprint describing phases, risk mitigation, and rollback steps for the directory reorganization.
- `documentation/directory_mapping_matrix.md` — Table mapping old paths to the new structure so contributors can update references confidently.
- `code/refactor_patchset_notes.md` — Running log of file moves, temporary toggles, and follow-up reminders (call out any short-lived compatibility code that must be removed).
- `validation/build_matrix.md` — Evidence (commands, platforms, results) that desktop, web, and CI builds succeed after each major migration phase.

Remove any temporary notes that no longer apply once the refactor is complete; the goal is to avoid leaving stale guidance behind.
