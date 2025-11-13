<!-- Machine Summary Block -->
{"file":"ai/tasks/glint_cli_platform_v1/artifacts/tests/determinism_validation_smoke.md","purpose":"Documents smoke tests for determinism validation in CLI platform.","exports":[],"depends_on":["cli/src/services/run_manifest_validator.cpp","cli/src/commands/render_command.cpp"],"notes":["determinism_verification","manifest_validation","checksum_testing"]}
<!-- Human Summary -->
Smoke test suite for validating run manifest schema compliance, checksum computation, and determinism enforcement in the Glint CLI platform.

# Determinism Validation Smoke Tests

## Overview

This test suite validates that the CLI platform correctly enforces determinism through:
1. Schema validation of run manifests
2. SHA-256 checksum computation and verification
3. Reproducible render outputs with identical inputs

## Test Environment

- **Target**: `glint render` command with determinism logging
- **Artifacts**: `renders/<name>/run.json` and `renders/<name>/run.json.sha256`
- **Schema Version**: 1.0.0

## Test Cases

### TC-01: Valid Manifest Schema

**Objective**: Verify that a valid run manifest passes schema validation.

**Setup**:
```bash
mkdir -p test_workspace/renders/test01
cd test_workspace
```

**Execution**:
```bash
glint render --input examples/models/sphere.obj --output renders/test01/output.png --width 256 --height 256 --name test01
```

**Expected Results**:
- ✓ Command exits with code 0 (Success)
- ✓ `renders/test01/run.json` exists
- ✓ `renders/test01/run.json.sha256` exists
- ✓ Manifest contains all required fields:
  - `schema_version`, `run_id`, `timestamp_utc`
  - `cli`, `platform`, `engine`, `determinism`, `outputs`
- ✓ No validation errors in output

**Validation**:
```bash
# Verify checksum file format
cat renders/test01/run.json.sha256
# Expected: <64-hex-chars> *run.json

# Verify checksum matches
sha256sum -c renders/test01/run.json.sha256
# Expected: run.json: OK
```

---

### TC-02: Manifest Missing Required Fields

**Objective**: Verify that manifests with missing required fields fail validation.

**Setup**:
Create a malformed manifest:
```json
{
  "schema_version": "1.0.0",
  "run_id": "test_run_001"
  // Missing: timestamp_utc, cli, platform, engine, determinism, outputs
}
```

**Execution**:
```bash
# Use inspect command to validate existing manifest
glint inspect test_workspace/renders/malformed/run.json
```

**Expected Results**:
- ✓ Command exits with code 2 (SchemaValidationError)
- ✓ Output contains validation errors:
  - "timestamp_utc: Required field missing [missing_field]"
  - "cli: Required field missing [missing_field]"
  - "platform: Required field missing [missing_field]"
  - "engine: Required field missing [missing_field]"
  - "determinism: Required field missing [missing_field]"
  - "outputs: Required field missing [missing_field]"

---

### TC-03: Manifest Invalid Field Types

**Objective**: Verify that manifests with invalid field types fail validation.

**Setup**:
Create a manifest with type errors:
```json
{
  "schema_version": 1.0,
  "run_id": 12345,
  "timestamp_utc": "2025-11-12T00:00:00.000Z",
  "cli": "should_be_object",
  "platform": [],
  "engine": {},
  "determinism": {},
  "outputs": {}
}
```

**Execution**:
```bash
glint inspect test_workspace/renders/type_errors/run.json
```

**Expected Results**:
- ✓ Command exits with code 2 (SchemaValidationError)
- ✓ Output contains type validation errors:
  - "schema_version: Must be a string [invalid_type]"
  - "run_id: Must be a string [invalid_type]"
  - "cli: Must be an object [invalid_type]"
  - "platform: Must be an object [invalid_type]"

---

### TC-04: Unsupported Schema Version

**Objective**: Verify that unsupported schema versions are rejected.

**Setup**:
```json
{
  "schema_version": "2.0.0",
  "run_id": "test_run_002",
  ...
}
```

**Execution**:
```bash
glint inspect test_workspace/renders/schema_v2/run.json
```

**Expected Results**:
- ✓ Command exits with code 2 (SchemaValidationError)
- ✓ Output contains:
  - "schema_version: Unsupported schema version (expected 1.0.0, got 2.0.0) [unsupported_version]"

---

### TC-05: Checksum Verification

**Objective**: Verify that manifest checksums can be computed and verified.

**Setup**:
```bash
glint render --input examples/models/sphere.obj --output renders/test05/output.png --name test05
```

**Execution**:
```bash
# Verify checksum
cd test_workspace/renders/test05
sha256sum -c run.json.sha256

# Tamper with manifest
sed -i 's/"run_id"/"run_id_modified"/' run.json

# Re-verify checksum (should fail)
sha256sum -c run.json.sha256
```

**Expected Results**:
- ✓ Initial checksum verification succeeds
- ✓ After tampering, checksum verification fails
- ✓ Tampered manifest can be detected by comparing checksums

---

### TC-06: Deterministic Render Comparison

**Objective**: Verify that identical inputs produce identical run manifests (excluding timestamps).

**Setup**:
```bash
# Render 1
glint render --input examples/models/sphere.obj --output renders/run1/output.png --width 512 --height 512 --name run1

# Wait 1 second to ensure different timestamp
sleep 1

# Render 2 (identical input)
glint render --input examples/models/sphere.obj --output renders/run2/output.png --width 512 --height 512 --name run2
```

**Execution**:
```bash
# Extract determinism-relevant fields (exclude timestamp, run_id)
jq 'del(.timestamp_utc, .run_id, .cli.arguments)' renders/run1/run.json > run1_normalized.json
jq 'del(.timestamp_utc, .run_id, .cli.arguments)' renders/run2/run.json > run2_normalized.json

# Compare normalized manifests
diff run1_normalized.json run2_normalized.json
```

**Expected Results**:
- ✓ `diff` exits with code 0 (no differences)
- ✓ Determinism fields match exactly:
  - `determinism.rng_seed`
  - `determinism.scene_digest`
  - `platform.os`, `platform.cpu`, `platform.gpu`
  - `engine.version`, `engine.modules`

---

### TC-07: CLI Structured Output

**Objective**: Verify that render command produces structured NDJSON output in --json mode.

**Execution**:
```bash
glint render --json --input examples/models/sphere.obj --output renders/test07/output.png --name test07 > output.ndjson
```

**Expected Results**:
- ✓ Output is valid NDJSON (newline-delimited JSON)
- ✓ Each line is valid JSON
- ✓ Output contains lifecycle events:
  - `{"event":"command_started","verb":"render",...}`
  - `{"event":"info","message":"Run manifest written to: renders/test07/run.json"}`
  - `{"event":"command_completed","exit_code":0,...}`

**Validation**:
```bash
# Validate each line is valid JSON
while IFS= read -r line; do
  echo "$line" | jq empty || echo "Invalid JSON: $line"
done < output.ndjson
```

---

### TC-08: Exit Code Determinism

**Objective**: Verify that commands produce deterministic exit codes.

**Execution Matrix**:

| Scenario | Command | Expected Exit Code | Exit Code Name |
|----------|---------|-------------------|----------------|
| Success | `glint render --input sphere.obj --output out.png` | 0 | Success |
| Missing file | `glint render --input missing.obj --output out.png` | 3 | FileNotFound |
| Invalid flag | `glint render --unknown-flag` | 5 | UnknownFlag |
| Schema error | `glint inspect invalid_manifest.json` | 2 | SchemaValidationError |

**Expected Results**:
- ✓ Exit codes match documented contract
- ✓ `--json` output includes `"exit_code"` and `"exit_code_name"`

---

## Automation Script

```bash
#!/bin/bash
# determinism_smoke_tests.sh

set -e

echo "=== Running Determinism Validation Smoke Tests ==="

TEST_DIR="test_workspace"
rm -rf "$TEST_DIR"
mkdir -p "$TEST_DIR"
cd "$TEST_DIR"

# TC-01: Valid Manifest
echo "[TC-01] Testing valid manifest schema..."
glint render --input ../examples/models/sphere.obj --output renders/test01/output.png --width 256 --height 256 --name test01
[ -f "renders/test01/run.json" ] || { echo "FAIL: run.json not created"; exit 1; }
[ -f "renders/test01/run.json.sha256" ] || { echo "FAIL: checksum file not created"; exit 1; }
echo "[TC-01] PASS"

# TC-05: Checksum Verification
echo "[TC-05] Testing checksum verification..."
cd renders/test01
sha256sum -c run.json.sha256 || { echo "FAIL: checksum verification failed"; exit 1; }
cd ../..
echo "[TC-05] PASS"

# TC-06: Deterministic Comparison
echo "[TC-06] Testing deterministic renders..."
glint render --input ../examples/models/sphere.obj --output renders/run1/output.png --width 512 --height 512 --name run1
sleep 1
glint render --input ../examples/models/sphere.obj --output renders/run2/output.png --width 512 --height 512 --name run2

jq 'del(.timestamp_utc, .run_id, .cli.arguments)' renders/run1/run.json > run1_norm.json
jq 'del(.timestamp_utc, .run_id, .cli.arguments)' renders/run2/run.json > run2_norm.json
diff run1_norm.json run2_norm.json || { echo "FAIL: renders not deterministic"; exit 1; }
echo "[TC-06] PASS"

echo "=== All tests passed ==="
```

---

## Expected Deliverables

1. **Automated test script** (`tests/determinism_smoke_tests.sh`)
2. **Sample manifests** for negative path testing
3. **CI integration** in `.github/workflows/cli_tests.yml`
4. **Test report template** for manual verification

---

## Success Criteria

- ✓ All test cases pass on supported platforms (Windows, Linux, macOS)
- ✓ Schema validation catches common error patterns
- ✓ Checksums are SHA-256 compliant (64 hex characters)
- ✓ Deterministic renders produce identical normalized manifests
- ✓ Exit codes follow documented contract
- ✓ Structured output is valid NDJSON

---

## Known Limitations

- **Engine integration pending**: Actual rendering logic is placeholder; manifests contain warnings
- **Platform metadata incomplete**: GPU/driver detection requires OpenGL context
- **Module registry**: Placeholder module/asset data until registry is implemented
- **Git revision**: Not yet captured in determinism metadata

---

*End of document.*
