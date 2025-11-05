<!-- Machine Summary Block -->
{"file":"ai/prompts/TASK_MODULE_CREATION_PROMPT.md","purpose":"Defines deterministic workflow for creating Glint3D task modules.","exports":["task_module_creation_contract"],"depends_on":["ai/current_index.json","ai/tasks/{task_id}"],"notes":["spec_version_v1_2","logging_required"]}
<!-- Human Summary -->
Instructions the automation agent follows to scaffold task modules: required inputs, directory layout, file content, logging, validation, and safeguards.

# AI TASK MODULE CREATION CONTRACT

## 1. PURPOSE
Define the exact, repeatable procedure for generating a Glint3D task module. All outputs must derive only from the supplied task metadata; no inferred or assumed values beyond what this spec states.

## 2. REQUIRED INPUT
Before execution, the AI must obtain:
- task_id (slug-safe string)
- title
- owner (optional; if omitted use "core_team")
- inputs.files array (may be empty but must exist)
- outputs.artifacts array (may be empty but must exist)
- acceptance array (may be empty but must exist)
- dependencies array (if omitted treat as [])
- optional metadata such as priority, notes, events

If any required element is missing, ambiguous, or conflicts with existing data -> HALT and report a blocker.

## 3. CORE RULES

AI SHALL:
- Use only the provided task details.
- Create the canonical folder and file structure exactly as defined.
- Populate every required file with valid, deterministic content (no placeholders).
- Append required events to `progress.ndjson` in chronological order.
- Register the task in `current_index.json` with status "pending".
- Add Machine Summary Block and human summary to any new Markdown file that lacks them.

AI MUST NOT:
- Invent fields, steps, dependencies, or artifacts.
- Create optional files unless explicitly instructed.
- Skip required files, directories, or logging steps.
- Continue after a validation failure.

## 4. DIRECTORY STRUCTURE

Create the following under `ai/tasks/{task_id}/`:

```
ai/tasks/{task_id}/
|- task.json
|- checklist.md
|- progress.ndjson
|- artifacts/
   |- README.md
   |- code/
   |- tests/
   |- documentation/
   |- validation/
```

Ensure empty directories contain a `.gitkeep` marker when needed.

## 5. FILE GENERATION ORDER

1. Scaffold the directory tree exactly as listed above.
2. Generate `task.json`:
   - Include all supplied metadata.
   - `status` = "pending".
   - `updated` = current ISO 8601 timestamp.
   - `events` array contains at least:

```json
[
  {"event":"task_started","timestamp":"<creation_ts>"},
  {"event":"task_completed","timestamp":null}
]
```

   - Preserve deterministic field ordering consistent with existing modules.
3. Generate `checklist.md`:
   - Prepend Machine Summary Block and human summary if absent.
   - Provide at least one phase heading and atomic, verifiable steps derived from acceptance criteria or supplied notes.
4. Generate `progress.ndjson` with the first event:

```json
{"ts":"<ISO_8601>","task_id":"<task_id>","event":"task_created","status":"pending","agent":"<agent>"}
```

5. Populate `artifacts/README.md` with the standard artifact overview template plus Machine Summary Block and human summary.
6. Place `.gitkeep` files in empty artifact subdirectories.
7. Update `current_index.json`:
   - `task_status[task_id] = {
       "status": "pending",
       "priority": "<provided or 'medium'>",
       "blocked_by": [],
       "completion_percentage": 0,
       "current_phase": "Not Started",
       "notes": "<optional provided notes>"
     }`
   - Append to `critical_path` if instructed.
   - Set `current_focus.active_task` when appropriate (only if no other active task or per explicit instruction).
8. Append `task_registered` event to `progress.ndjson` after the index update:

```json
{"ts":"<ISO_8601>","task_id":"<task_id>","event":"task_registered","status":"pending","agent":"<agent>"}
```

9. Run full validation (section 7). If any check fails, stop and report.
10. Report: `Task module <task_id> created and registered successfully.`

## 6. LOGGING REQUIREMENTS

- `progress.ndjson` must contain newline-delimited JSON (no pretty printing).
- Required events: `task_created`, `task_registered` (additional events allowed if provided).
- Every event object includes `ts`, `task_id`, `event`, `agent`, and optional `details`.
- Timestamps must monotonically increase within the file.

## 7. VALIDATION CHECKLIST

Before announcing success confirm:
- All required directories/files exist and are non-empty (aside from `.gitkeep`).
- `task.json` is valid JSON and mirrors the provided metadata.
- `checklist.md` aligns with task scope and contains actionable steps.
- `progress.ndjson` parses line-by-line as JSON and events are chronological.
- `current_index.json` entry matches the new task definition exactly.
- Machine Summary Block + human summary present where mandated.
- No extraneous files were created.

## 8. ANTI-HALLUCINATION SAFEGUARDS

- Request clarification or stop if any input is missing or unclear.
- Never assume acceptance criteria, dependencies, or artifacts.
- Do not fabricate timestamps or agent identifiers.
- If validation fails, halt and emit a failure report instead of proceeding.

# END OF SPEC v1.2
