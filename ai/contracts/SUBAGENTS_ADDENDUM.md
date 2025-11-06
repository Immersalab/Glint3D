# AI SUBAGENTS ADDENDUM — GLINT3D (v1.0)

**Extends:** [AI EXECUTION CONTRACT — GLINT3D TASK MODULE SYSTEM](../FOR_MACHINES.md) (v2.2)

**Purpose:** Deterministic orchestration of multiple AI agents where Codex plans, delegates, and audits; Claude executes bounded, well-specified subtasks.

**Canonical References:** §0 (Behavioral), §1 (No Hallucination), §2 (Source of Truth), §3 (Deterministic Event Logging), §4 (Execution Protocol), §6 (Artifact Validation), §7 (Safety), §8 (Logging Format), §9 (Context Reanchoring), §11 (End State).

---

## A. ROLES & AUTHORITY

### A.1 Role Definitions

#### **codex** (orchestrator/builder/auditor)
- Sole authority for planning, decomposition, auditing, merges, and writes.
- May delegate bounded subtasks to workers (Claude).
- Applies or rejects patches after validation (§F).

#### **claude** (worker)
- Executes well-defined subtasks: docs, tests, micro-refactors, localized code, reviews.
- Produces patches and/or artifacts; writes are token-gated (§D).
- Must not spawn further agents.

### A.2 Routing Policy
- **codex** handles open-ended, cross-file, architectural, or perf-critical work.
- **claude** handles deterministic, localized, clerical, or spec-tight work.
- Any ambiguity → stop per §1 and log `blocker_encountered`.

---

## B. REGISTRY & CAPABILITIES

### B.1 Agent Registry (authoritative on disk)

**Location:** `ai/agents/registry.json`

```json
{
  "agents": [
    {
      "id": "claude",
      "cmd": "agents/claude-cli",
      "version": ">=2.0.34",
      "capabilities": [
        "test-author",
        "doc-writer",
        "review",
        "micro-refactor",
        "localized-impl"
      ]
    },
    {
      "id": "codex",
      "cmd": "agents/codex-cli",
      "version": ">=1.0.0",
      "capabilities": [
        "plan",
        "orchestrate",
        "codegen",
        "audit",
        "merge"
      ]
    }
  ]
}
```

**Requirements:**
- Codex must verify registry presence and version range before any invocation.

---

## C. SUBAGENT INVOCATION (NDJSON I/O)

### C.1 Call Envelope (STDIN to worker — exactly two lines)

#### Line 1: context
```json
{
  "ts": "<ISO8601>",
  "task_id": "<id>",
  "type": "context",
  "payload": {
    "repo_root": ".",
    "branch": "<name>",
    "readonly": ["engine/include/**", "third_party/**"],
    "visible": ["**/*"],
    "call_id": "<sha256(task_id + role + inputs_hash)>"
  }
}
```

#### Line 2: prompt
```json
{
  "ts": "<ISO8601>",
  "task_id": "<id>",
  "type": "prompt",
  "payload": {
    "role": "test-author|doc-writer|review|micro-refactor|localized-impl",
    "goal": "<single-sentence>",
    "constraints": [
      "ground-in-provided-files-only",
      "no-new-deps",
      "no-speculation"
    ],
    "inputs": {
      "paths": ["engine/src/foo.cpp", "engine/include/foo.hpp"],
      "blobs": [{"name": "checklist.md", "text": "..."}]
    },
    "expected": {
      "schema": "json",
      "fields": ["plan", "diffs", "artifacts", "risks", "notes", "metrics"]
    },
    "write_scope": "none|token:<write_token_id>",
    "timeouts": {
      "soft_s": 60,
      "hard_s": 240
    },
    "budgets": {
      "max_tokens": 8192
    }
  }
}
```

### C.2 Worker Output (STDOUT — exactly one line)

```json
{
  "ts": "<ISO8601>",
  "task_id": "<id>",
  "status": "ok|error",
  "agent_id": "claude",
  "result": {
    "plan": [{"step": "...", "why": "..."}],
    "diffs": [{"file": "path", "patch": "<unified-diff>"}],
    "artifacts": [{"name": "test_plan.md", "type": "text/markdown", "content": "..."}],
    "metrics": {"coverage_delta": 0.10, "files_touched": 2},
    "risks": ["edge case X"],
    "notes": ["..."]
  },
  "error": {
    "message": "...",
    "kind": "retryable|hard"
  }
}
```

**Exit Codes:**
- `0` = success
- `1` = retryable error
- `2` = hard error

**Validation:**
- If output is not valid JSON → Codex logs `subagent_failed(kind=format)` and HALTs per §5.

### C.3 Orchestration Events (append to §8 Allowed events)

**New Events:**
- `subagent_invoked`
- `subagent_succeeded`
- `subagent_failed`
- `subagent_retried`
- `subagent_diffs_applied`

**Examples** (`progress.ndjson`):

```json
{"ts":"<ISO8601>","task_id":"<id>","event":"subagent_invoked","status":"pending","agent":"codex","details":{"subagent":"claude","role":"doc-writer","call_id":"H..."}}
{"ts":"<ISO8601>","task_id":"<id>","event":"subagent_succeeded","status":"ok","agent":"codex","details":{"subagent":"claude","artifacts":["ai/tmp/<id>/test_plan.md"],"diffs":2}}
{"ts":"<ISO8601>","task_id":"<id>","event":"subagent_diffs_applied","status":"ok","agent":"codex","details":{"files":["docs/api/foo.md"],"commit":"<sha>"}}
```

---

## D. WRITE TOKENS (SCOPED WRITES)

### D.1 Token Store

**Location:** `ai/agents/write_tokens.json`

```json
{
  "tokens": [
    {
      "id": "W123",
      "paths": ["tests/**", "docs/**"],
      "expires": "<ISO8601>",
      "one_shot": true
    }
  ]
}
```

### D.2 Enforcement

- **Default** `write_scope="none"`: worker returns diffs only; Codex applies.
- **With** `token:<id>`: worker may propose diffs only within allowed paths; Codex rejects out-of-scope hunks.

---

## E. PLANNING & QUEUEING (Codex)

### E.1 Plan File

**Location:** `ai/tasks/{task_id}/plan.json`

```json
{
  "task_id": "<id>",
  "created": "<ISO8601>",
  "subtasks": [
    {
      "id": "T1",
      "role": "doc-writer",
      "goal": "Document foo.hpp",
      "inputs": ["engine/include/foo.hpp"],
      "acceptance": ["Doxygen complete", "no signature drift"],
      "write_scope": "docs/**"
    },
    {
      "id": "T2",
      "role": "test-author",
      "goal": "Add tests for Bar()",
      "inputs": ["engine/src/bar.cpp"],
      "acceptance": ["branch>=0.8"],
      "write_scope": "tests/**"
    }
  ]
}
```

### E.2 Queue Discipline

- Default FIFO with optional dependency edges → topo order.
- Codex mirrors subtasks to `checklist.md` (one `[ ]` per subtask) per §4.

---

## F. AUDIT, VALIDATE, APPLY (Codex-ONLY)

### F.1 Patch Validation

**Codex must:**
1. Verify `call_id` idempotency (dedupe).
2. Validate unified diffs: file paths within token scope (if any), no binary/blobs, no path traversal.
3. Enforce §1 (no new files/artifacts unless listed).
4. Run format/lint/license checks.
5. Build + run tests (twice to detect flakiness).
6. If headers changed, run §0C Doxygen checks.

**On Success:**
- Apply diffs, commit, log `subagent_diffs_applied`.

**On Failure:**
- Log `subagent_failed` + HALT per §5.

### F.2 Role-specific Acceptance

- **test-author**: measured coverage meets/raises target; tests hermetic (no net/proc leaks).
- **doc-writer**: §0C satisfied; docs match signatures; docs build clean (warnings→errors).
- **micro-refactor**: AST-invariant (no public API change) unless plan explicitly allows.
- **review**: issues must cite exact lines/rules; if diffs included, treat as micro-refactor.

---

## G. FAILURE & RETRY (Refines §5)

- `status=error, kind=retryable` → one constrained retry (tighten scope/timeouts/inputs).
- On second failure or `hard` → log `blocker_encountered` and HALT.
- Spec drift (out-of-scope paths, new deps) → hard fail.
- Uncited assumptions (referencing files not in `inputs.paths`/`blobs`) → hard fail.

---

## H. SANDBOX, BUDGETS, PROVENANCE (Refines §7)

### H.1 Sandbox
- Worker processes: no network, FS read limited to `visible[]`, writes disabled unless token present.
- Redact env vars by default; allowlist as needed.

### H.2 Provenance
Each exchange recorded in `ai/exchanges/<task_id>/<call_id>/`:
```
context.ndjson
prompt.ndjson
stdout.ndjson
stderr.log
patches/
artifacts/
```
Must be sufficient to fully replay result.

---

## I. MACHINE-FRIENDLY PROMPTING

- Strict JSON/NDJSON only; forbid free-form prose in worker output.
- Prefer file references over large in-prompt blobs; include diffs or minimal excerpts.
- Always restate role and objective in `payload.role` / `payload.goal`.
- Use delimiters or separate `blobs[]` for any embedded text.

---

## J. EXAMPLE FLOWS

### J.1 Docs Subtask (claude → codex audit)

1. Codex appends `subagent_invoked`.
2. Sends context + prompt(`role=doc-writer, write_scope=token:W123`).
3. Claude returns artifacts + diffs in `docs/**`.
4. Codex runs §F (doc build + §0C), applies, commits, logs `subagent_succeeded` + `subagent_diffs_applied` + marks checklist `[x]`.

### J.2 Tests Subtask (claude → codex audit)

1. `role=test-author, write_scope=tests/**`.
2. Claude returns tests + `coverage_expectation` in metrics.
3. Codex executes tests twice; compares coverage (±1% tolerance); applies or HALTs.

### J.3 Localized Impl (claude) + Post Audit (codex)

1. `role=localized-impl, write_scope=token:WXYZ` constrained to `engine/modules/gizmo/**`.
2. Claude provides limited diffs; Codex validates, builds, runs tests, audits edge cases; applies.

---

## K. CONTRACT COMPATIBILITY

This addendum does not override v2.2; it binds to:

- **§1 No Hallucination**: Workers must reject missing inputs.
- **§3 Deterministic Logging**: All subagent events are additional.
- **§4 Execution Protocol**: Codex follows same step gating; worker steps are nested under a parent step.
- **§6 Artifact Validation**: Unchanged; worker artifacts validated by Codex.
- **§11 End State**: Unchanged; Codex is final authority to mark complete.

---

## L. MINIMAL CLI SHIMS (REFERENCE)

### `agents/claude-cli` (pseudocode)

```bash
read line1(context); read line2(prompt);
validate schema → if invalid: exit 2 with error JSON
execute role handler → produce single-line JSON result
stdout.write(result); exit 0
```

### `agents/codex-cli`

The orchestrator:
1. Loads plan, selects next subtask (§E).
2. Emits `subagent_invoked`, spawns worker with §C envelopes.
3. Consumes worker JSON; runs §F; commits; advances checklist; logs completion.

---

## M. SUMMARY

This addendum provides deterministic, AI-friendly dual-agent operations with minimal ceremony:

- **Codex** plans, delegates, and audits.
- **Claude** executes within tight bounds.
- Everything is logged, replayable, and token-efficient.

---

**END OF ADDENDUM v1.0**
