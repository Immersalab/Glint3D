# AI EXECUTION CONTRACT — GLINT3D TASK MODULE SYSTEM

# This file defines the deterministic protocol an AI agent must follow during any task execution in the Glint3D project.

## 0. BEHAVIORAL CONTRACT

PRIORITY_ORDER:
1. Correctness
2. Clarity
3. Simplicity (YAGNI + KISS)
4. DRY
5. Performance (only if required by acceptance criteria)

AI_SHALL:
- Follow all protocol instructions in this document without deviation.
- Read authoritative state from disk at the start of every execution cycle.
- Never assume or fabricate context, steps, outputs, or artifacts.
- Follow steps strictly in order; no skipping, merging, or reordering.
- Immediately STOP and REPORT if ambiguity, missing context, or blocker is detected.
- Always re-anchor to current_index.json and progress.ndjson before execution.
- Always log structured events to progress.ndjson in deterministic order.
- Validate every step before marking complete.

AI_MUST_NOT:
- Hallucinate, infer, or invent missing steps, acceptance criteria, task structure, file paths, outputs, or context.
- Add steps or modify task behavior not explicitly listed in task.json, checklist.md, or acceptance criteria.
- Modify unrelated files.
- Proceed after any failed step.
- Ignore or silence any errors.

## 0A. MACHINE SUMMARY BLOCK PROTOCOL

AI_SHALL:
- Before reading or modifying any source file, locate the **Machine Summary Block** at the very top of the file (immediately preceding code or directives) and parse it for authoritative context.
- If the Machine Summary Block is missing, create one **before proceeding further**. The block must start with a comment line containing `Machine Summary Block` and be followed by one or more newline-delimited JSON entries (NDJSON) optimized for machine parsing.
- Keep the block concise (<= 3 JSON lines unless justified) and update it whenever file responsibilities, exports, or key dependencies change.

Recommended JSON fields:
- `file`: canonical path or filename.
- `purpose`: single-sentence description of what the file declares or implements.
- `exports`: array of primary types, functions, or resources defined.
- `depends_on`: array of critical dependencies or forward declarations.
- `notes`: short array highlighting behavioral nuances, invariants, or special protocols.

AI_MUST:
- Ensure the JSON is valid UTF-8 ASCII, double-quoted, and free of trailing commas.
- Preserve existing Machine Summary Blocks when present, editing fields in place to keep them accurate.
- Prefer lower_snake_case field names for consistency.

 AI_MUST_NOT:
 - Place the block after code, include verbose prose, or omit it when creating new files.

## 0B. HUMAN SUMMARY GUIDELINES

AI_SHALL:
- Maintain a concise human-readable summary comment directly beneath the Machine Summary Block.
- Keep the summary scoped to the file's purpose, key responsibilities, or architecture context; avoid duplicating the NDJSON content verbatim.
- Update the summary whenever behavior, exports, or design intent materially changes.

AI_MUST:
- Prefer brief paragraphs or short bullet fragments over exhaustive walkthroughs.
- Remove stale sections if they no longer reflect the file.
- Keep formatting consistent with surrounding codebase conventions (e.g., Doxygen `@file` blocks for headers).

AI_MUST_NOT:
- Allow the human summary to contradict the Machine Summary Block.
- Expand the summary with tutorial-level detail or changelog notes.

## 0C. DOXYGEN DOCUMENTATION PROTOCOL

AI_SHALL:
- Ensure all public header files (in `engine/include/`) use Doxygen-compliant documentation format.
- After completing any task that modifies or creates header files, verify that all comments are up to date and properly formatted for Doxygen.
- Check documentation completeness as part of task validation before marking any step complete.

Required Doxygen Elements for Headers:
- `@file` directive at the top of each header file with brief description
- `@brief` for all public classes, structs, functions, and enums
- `@param` for all function parameters
- `@return` for all functions with return values
- `@note` for important behavioral notes or invariants
- `@warning` for critical usage warnings
- `@see` for related functions or classes when appropriate

AI_MUST:
- Use `///` or `/** */` style comments for Doxygen blocks
- Place brief descriptions on the same line as `@brief` or immediately after
- Document all public APIs before marking a step complete
- Verify documentation matches actual implementation
- Update existing documentation when modifying function signatures or behavior

AI_MUST_NOT:
- Leave public APIs undocumented
- Use generic placeholder comments (e.g., "TODO: document this")
- Mark a task complete if header documentation is missing or outdated
- Skip documentation updates when modifying existing code

Documentation Verification Checklist:
1. All public classes/structs have `@brief` and purpose description
2. All public functions have `@brief`, `@param`, and `@return` (if applicable)
3. Complex algorithms or behaviors have `@note` explaining approach
4. Edge cases or limitations documented with `@warning`
5. All parameters and return values accurately described
6. Cross-references added with `@see` where helpful

## 1. NO HALLUCINATION POLICY

- IF information is not found in:
  - current_index.json
  - task.json
  - checklist.md
  - progress.ndjson
  - artifacts/README.md
  THEN it **does not exist** for execution purposes.

- AI MUST NOT:
  - Invent names, steps, acceptance criteria, or dependencies.
  - Fill gaps with guesses.
  - Infer missing instructions based on patterns.
  - Generate artifacts not listed in outputs.artifacts.

- IF required data is missing:
  - STOP immediately.
  - LOG a blocker_encountered event.
  - REPORT: “Missing data — execution paused.”
  - WAIT for explicit human instruction.

## 2. SOURCE OF TRUTH

Authoritative Files:
- ai/tasks/current_index.json
- ai/tasks/{task_id}/task.json
- ai/tasks/{task_id}/checklist.md
- ai/tasks/{task_id}/progress.ndjson
- ai/tasks/{task_id}/artifacts/README.md
- Optional: coverage.md, mapping.md, README.md

No other source is valid.

## 3. DETERMINISTIC EVENT LOGGING

- progress.ndjson is the **single source of truth** for execution history.
- Events MUST be appended in **strict chronological order**.
- All steps MUST be anchored to the **latest event in progress.ndjson** before continuing.

Event logging rules:
1. Read progress.ndjson before each action. Identify last event.
2. Determine next step from checklist.md + progress.ndjson.
3. After executing any step, append exactly one event line with:
   - ts (ISO 8601)
   - task_id
   - event type
   - status
   - agent
   - optional fields (step, error, details)
4. Never rewrite, delete, or reorder events.
5. Never emit duplicate step_completed for the same step.

Required event sequence for each task:
1. task_started
2. [step_completed or step_failed for each checklist item in order]
3. acceptance_failed or task_completed
4. task_completed (if successful)

## 4. EXECUTION PROTOCOL

### STEP 1: READ CURRENT INDEX

Read: ai/tasks/current_index.json
- If active_task == null → REPORT "No active task" → HALT
- If status == "blocked" → REPORT blocker → HALT
- If status == "completed" → REPORT "Already complete" → REQUEST next task
- Else → PROCEED to STEP 2

### STEP 2: LOAD CONTEXT

Read:
- task.json
- checklist.md
- progress.ndjson
- artifacts/README.md
(Optional: coverage.md, mapping.md, README.md)

Anchor to latest event in progress.ndjson.

### STEP 3: DETERMINE NEXT ACTION

pending_steps = all checklist items with [ ]
IF pending_steps is empty:
  action = validate_completion
ELSE:
  action = execute_step(next_pending)

### STEP 4: EXECUTE STEP

Announce:
- "Working on: {step_name}"
- "Description: {step_description}"

Perform:
- Modify only inputs.files
- Execute required commands
- Generate expected artifacts

Validate:
- No compilation errors
- Tests pass
- Expected outputs exist and match spec
- **Verify Doxygen Documentation** (if headers modified):
  - Check all modified header files for Doxygen compliance
  - Ensure all new/modified public APIs are documented
  - Verify documentation matches implementation
  - Run documentation verification checklist from §0C

If success:
- Mark step [x] in checklist.md
- Append step_completed event (single line JSON)
If failure:
- Append step_failed event
- HALT execution

### STEP 5: VALIDATE COMPLETION

For each acceptance criterion:
- If fails → append acceptance_failed event → RETURN to execution
- If passes → continue

Documentation Validation:
- **Verify all modified header files have up-to-date Doxygen documentation**
- Run through documentation verification checklist from §0C
- If any header lacks proper documentation → append acceptance_failed event → RETURN to execution
- If documentation is incomplete or outdated → HALT and update before proceeding

### STEP 6: MARK TASK COMPLETE

- Update task.json status to completed
- Append task_completed event
- Update current_index.json:
  - task_status[active_task].status = completed
  - task_status[active_task].completion_date = ISO timestamp
  - current_focus.active_task = next_task
- REPORT completion

## 5. FAILURE HANDLING

### Build or Test Failure
- Log build_failed or test_failed
- Attempt one remediation
- If still fails → HALT

### Blockers
- Log blocker_encountered
- Update status to blocked
- REPORT blocker
- WAIT

### Acceptance Failures
- Log acceptance_failed
- Do not mark task completed
- Add remediation or WAIT

## 6. ARTIFACT VALIDATION

- All artifacts listed in outputs.artifacts must be generated deterministically.
- Validate file existence, size, and format.
- Regenerate and log failures if validation fails.
- Log generation in progress.ndjson.

## 7. SAFETY CONSTRAINTS

- Respect all rate_limits
- Enforce sandbox restrictions when active
- Abort immediately on safety violation

## 8. LOGGING FORMAT

progress.ndjson must contain **newline-delimited, non-pretty JSON** with required fields:
- ts
- task_id
- event
- status
- agent
Optional: step, phase, details, error

Allowed events:
task_started, task_completed, step_completed, step_failed,
phase_completed, build_succeeded, build_failed,
test_passed, test_failed, blocker_encountered,
blocker_resolved, acceptance_failed.

## 9. CONTEXT REANCHORING

Before every step:
- Re-read current_index.json and progress.ndjson
- Anchor to latest event
- Abort if context changed or task blocked

## 10. ANTI-HALLUCINATION GUARDRAILS

- All reasoning must be grounded in current task files.
- If the model “does not know,” it must explicitly state:
  “NO VALID CONTEXT — EXECUTION HALTED”
- No guessing or auto-completion of missing information.
- No soft interpretations of ambiguous instructions.

## 11. END STATE

A task is only DONE if:
- All checklist steps are [x]
- All acceptance criteria are met
- All artifacts exist and validated
- **All modified header files have complete, up-to-date Doxygen documentation**
- **Documentation verification checklist (§0C) passed for all headers**
- task.json marked completed
- current_index.json updated
- task_completed event logged

NO EXCEPTIONS.

# END OF SPEC v2.2
