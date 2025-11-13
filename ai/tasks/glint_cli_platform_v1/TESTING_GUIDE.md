<!-- Machine Summary Block -->
{"file":"ai/tasks/glint_cli_platform_v1/TESTING_GUIDE.md","purpose":"Practical hands-on testing guide for Glint CLI Platform v1.","exports":[],"depends_on":["builds/desktop/cmake/Debug/glint.exe"],"notes":["manual_testing","smoke_testing","quick_start"]}
<!-- Human Summary -->
Step-by-step testing guide for manually validating all CLI commands and features.

# Glint CLI Platform v1 - Testing Guide

## Quick Start Testing (5 minutes)

### Prerequisites

1. **Build completed successfully**:
```bash
cd D:\ahoqp1\Repositories\Glint3D
cmake --build builds/desktop/cmake --config Debug
```

2. **Executable location**:
```
builds/desktop/cmake/Debug/glint.exe
```

---

## Test Suite 1: Basic Command Execution

### Test 1.1: Help and Version

```bash
# Test: Verify executable runs
cd D:\ahoqp1\Repositories\Glint3D
.\builds\desktop\cmake\Debug\glint.exe --help

# Expected: Should show help text (or error about unrecognized flag)
# Exit code: 0 or 5 (depending on if legacy --help is wired)

# Test: Check version
.\builds\desktop\cmake\Debug\glint.exe --version

# Expected: Version info or error
```

### Test 1.2: Command Recognition

```bash
# Test: List of commands (try each verb)
.\builds\desktop\cmake\Debug\glint.exe init
.\builds\desktop\cmake\Debug\glint.exe validate
.\builds\desktop\cmake\Debug\glint.exe inspect
.\builds\desktop\cmake\Debug\glint.exe render
.\builds\desktop\cmake\Debug\glint.exe config
.\builds\desktop\cmake\Debug\glint.exe clean
.\builds\desktop\cmake\Debug\glint.exe doctor
.\builds\desktop\cmake\Debug\glint.exe modules
.\builds\desktop\cmake\Debug\glint.exe assets

# Expected: Each should execute (may fail with missing arguments, that's OK)
# Exit codes: Should be defined codes (0, 2, 3, 4, 5, 6, 7)
```

### Test 1.3: Stub Commands

```bash
# Test: Watch command (stub)
.\builds\desktop\cmake\Debug\glint.exe watch

# Expected: Error message with:
# - "glint watch: Command not yet implemented"
# - Workaround guidance (use watchexec)
# Exit code: 4 (RuntimeError)

# Test: Profile command (stub)
.\builds\desktop\cmake\Debug\glint.exe profile

# Expected: Similar stub message with profiler suggestions
# Exit code: 4 (RuntimeError)

# Test: Convert command (stub)
.\builds\desktop\cmake\Debug\glint.exe convert

# Expected: Similar stub message with conversion tool suggestions
# Exit code: 4 (RuntimeError)
```

---

## Test Suite 2: Clean Command (Safest to Test First)

### Test 2.1: Dry Run Mode

```bash
cd D:\ahoqp1\Repositories\Glint3D

# Create test directories
mkdir -p renders/test_output
mkdir -p .glint/cache
echo "test" > renders/test_output/test.png

# Test: Clean with dry run
.\builds\desktop\cmake\Debug\glint.exe clean --dry-run

# Expected output:
# - "Dry run mode: no files will be deleted"
# - Summary of what would be removed
# Exit code: 0

# Verify: Files still exist
ls renders/test_output/test.png
# Should still exist
```

### Test 2.2: Selective Cleaning

```bash
# Test: Clean renders only
.\builds\desktop\cmake\Debug\glint.exe clean --renders --verbose

# Expected:
# - Removes renders/ directory
# - Shows count of items removed
# Exit code: 0

# Test: Clean cache only
.\builds\desktop\cmake\Debug\glint.exe clean --cache --verbose

# Expected:
# - Removes .glint/cache/ directory
# Exit code: 0
```

---

## Test Suite 3: Inspect Command

### Test 3.1: Inspect Scene File

```bash
cd D:\ahoqp1\Repositories\Glint3D

# Test: Inspect a scene file (if you have one)
# Find an example file first
ls engine/assets/models/  # or wherever test models are

# Example (adjust path to actual file):
.\builds\desktop\cmake\Debug\glint.exe inspect engine/assets/models/sphere.obj

# Expected output:
# - Scene file: sphere.obj
# - Format: obj
# - Size: X bytes
# - Note about engine integration pending
# Exit code: 0
```

### Test 3.2: Inspect Non-existent File

```bash
# Test: File not found error
.\builds\desktop\cmake\Debug\glint.exe inspect nonexistent.obj

# Expected:
# - Error message: "Target file not found: nonexistent.obj"
# Exit code: 3 (FileNotFound)
```

### Test 3.3: Inspect with JSON Output

```bash
# Test: Structured output
.\builds\desktop\cmake\Debug\glint.exe inspect --json engine/assets/models/sphere.obj

# Expected:
# - NDJSON output (newline-delimited JSON)
# - Each line should be valid JSON
# - Events: command_started, info, command_completed
# Exit code: 0
```

---

## Test Suite 4: Validate Command

### Test 4.1: Basic Validation

```bash
cd D:\ahoqp1\Repositories\Glint3D

# Test: Validate current directory (should fail if no manifest)
.\builds\desktop\cmake\Debug\glint.exe validate

# Expected:
# - May fail if no glint.project.json exists
# - Informative error message
# Exit code: 3 or 4 (depending on error type)
```

---

## Test Suite 5: Config Command

### Test 5.1: View Configuration

```bash
# Test: Show current config
.\builds\desktop\cmake\Debug\glint.exe config

# Expected:
# - Lists configuration settings
# - Or message about no config found
# Exit code: 0
```

---

## Test Suite 6: Doctor Command

### Test 6.1: Environment Diagnostics

```bash
# Test: Run diagnostics
.\builds\desktop\cmake\Debug\glint.exe doctor

# Expected output:
# - Engine version
# - Module status
# - Dependency checks (OpenGL, Assimp, etc.)
# - Configuration status
# Exit code: 0
```

---

## Test Suite 7: Modules Command

### Test 7.1: List Modules

```bash
# Test: List available modules
.\builds\desktop\cmake\Debug\glint.exe modules list

# Expected:
# - Lists engine modules (raytracing, gizmos, etc.)
# - Shows enabled/disabled status
# Exit code: 0
```

### Test 7.2: Enable/Disable Module

```bash
# Test: Enable a module
.\builds\desktop\cmake\Debug\glint.exe modules enable raytracing

# Expected:
# - Success message or error if already enabled
# Exit code: 0 or error code

# Test: Disable a module
.\builds\desktop\cmake\Debug\glint.exe modules disable gizmos

# Expected:
# - Success message
# Exit code: 0
```

---

## Test Suite 8: Init Command

### Test 8.1: Initialize New Project

```bash
# Create test directory
mkdir D:\test_glint_project
cd D:\test_glint_project

# Test: Initialize with default template
D:\ahoqp1\Repositories\Glint3D\builds\desktop\cmake\Debug\glint.exe init

# Expected:
# - Creates glint.project.json
# - Creates .glint/config.json
# - Creates directory structure (scenes/, renders/, ops/)
# Exit code: 0

# Verify:
ls glint.project.json
ls .glint/config.json
```

### Test 8.2: Initialize with Template

```bash
cd D:\test_glint_project_pbr
D:\ahoqp1\Repositories\Glint3D\builds\desktop\cmake\Debug\glint.exe init --template pbr

# Expected:
# - Creates project with PBR template
# Exit code: 0
```

---

## Test Suite 9: Render Command (Most Complex)

### Test 9.1: Basic Render (Will Likely Fail - Engine Integration Pending)

```bash
cd D:\ahoqp1\Repositories\Glint3D

# Test: Render with missing input (should fail gracefully)
.\builds\desktop\cmake\Debug\glint.exe render --output test.png

# Expected:
# - Error: "Must specify either --input or --ops"
# Exit code: 5 (UnknownFlag)

# Test: Render with non-existent input
.\builds\desktop\cmake\Debug\glint.exe render --input missing.obj --output test.png

# Expected:
# - Error: "Input file not found: missing.obj"
# Exit code: 3 (FileNotFound)
```

### Test 9.2: Render with Real File (If Available)

```bash
# Find a test model
ls examples/models/  # Adjust path as needed

# Test: Render attempt
.\builds\desktop\cmake\Debug\glint.exe render --input examples/models/sphere.obj --output test_render.png --width 256 --height 256 --name test_render_001

# Expected:
# - May succeed with placeholder logic
# - Creates renders/test_render_001/run.json
# - Creates renders/test_render_001/run.json.sha256
# - Warning: "Render command integration with engine is pending"
# Exit code: 0 (even with placeholder)

# Verify manifest was created:
ls renders/test_render_001/run.json
cat renders/test_render_001/run.json
```

### Test 9.3: Verify Run Manifest Structure

```bash
# Test: Inspect the generated manifest
.\builds\desktop\cmake\Debug\glint.exe inspect renders/test_render_001/run.json

# Expected:
# - Shows run manifest details
# - Run ID, timestamp, platform info, etc.
# Exit code: 0

# Test: Verify checksum file
cat renders/test_render_001/run.json.sha256

# Expected format:
# <64-hex-chars> *run.json
```

### Test 9.4: Render with JSON Output

```bash
# Test: Structured output mode
.\builds\desktop\cmake\Debug\glint.exe render --json --input examples/models/sphere.obj --output test_json.png --name test_json

# Expected:
# - NDJSON output (each line is valid JSON)
# - Events: command_started, info messages, command_completed
# Exit code: 0

# Verify JSON validity:
# Pipe through jq if available:
.\builds\desktop\cmake\Debug\glint.exe render --json --input examples/models/sphere.obj --output test_json2.png --name test_json2 | jq .
```

---

## Test Suite 10: Exit Codes Verification

### Test 10.1: Success Exit Code

```bash
# Test: Successful command
.\builds\desktop\cmake\Debug\glint.exe clean --dry-run
echo $LASTEXITCODE  # Windows PowerShell
# or
echo $?  # Bash

# Expected: 0
```

### Test 10.2: FileNotFound Exit Code

```bash
# Test: File not found
.\builds\desktop\cmake\Debug\glint.exe inspect missing.obj
echo $LASTEXITCODE  # Windows PowerShell

# Expected: 3
```

### Test 10.3: UnknownFlag Exit Code

```bash
# Test: Unknown flag
.\builds\desktop\cmake\Debug\glint.exe render --unknown-flag
echo $LASTEXITCODE  # Windows PowerShell

# Expected: 5
```

### Test 10.4: RuntimeError Exit Code

```bash
# Test: Stub command
.\builds\desktop\cmake\Debug\glint.exe watch
echo $LASTEXITCODE  # Windows PowerShell

# Expected: 4
```

---

## Test Suite 11: Global Flags

### Test 11.1: Verbosity Flag

```bash
# Test: Debug verbosity
.\builds\desktop\cmake\Debug\glint.exe --verbosity debug clean --dry-run

# Expected:
# - More verbose output (if logger is wired to commands)
# Exit code: 0

# Test: Quiet mode
.\builds\desktop\cmake\Debug\glint.exe --verbosity quiet clean --dry-run

# Expected:
# - Minimal output
# Exit code: 0
```

### Test 11.2: Project Flag

```bash
# Test: Specify project path
.\builds\desktop\cmake\Debug\glint.exe --project D:\test_glint_project\glint.project.json validate

# Expected:
# - Uses specified project
# Exit code: Varies
```

---

## Automated Quick Test Script

Create a file `quick_test.bat` (Windows) or `quick_test.sh` (Linux):

### Windows (quick_test.bat):
```batch
@echo off
echo ===================================
echo Glint CLI Quick Test Suite
echo ===================================
echo.

set GLINT=D:\ahoqp1\Repositories\Glint3D\builds\desktop\cmake\Debug\glint.exe

echo Test 1: Clean (dry run)
%GLINT% clean --dry-run
echo Exit code: %ERRORLEVEL%
echo.

echo Test 2: Doctor
%GLINT% doctor
echo Exit code: %ERRORLEVEL%
echo.

echo Test 3: Modules list
%GLINT% modules list
echo Exit code: %ERRORLEVEL%
echo.

echo Test 4: Inspect non-existent file (should fail with code 3)
%GLINT% inspect missing.obj
echo Exit code: %ERRORLEVEL%
echo.

echo Test 5: Watch stub (should fail with code 4)
%GLINT% watch
echo Exit code: %ERRORLEVEL%
echo.

echo ===================================
echo Quick test complete!
echo ===================================
```

### Linux/Mac (quick_test.sh):
```bash
#!/bin/bash

echo "==================================="
echo "Glint CLI Quick Test Suite"
echo "==================================="
echo

GLINT="./builds/desktop/cmake/Debug/glint.exe"

echo "Test 1: Clean (dry run)"
$GLINT clean --dry-run
echo "Exit code: $?"
echo

echo "Test 2: Doctor"
$GLINT doctor
echo "Exit code: $?"
echo

echo "Test 3: Modules list"
$GLINT modules list
echo "Exit code: $?"
echo

echo "Test 4: Inspect non-existent file (should fail with code 3)"
$GLINT inspect missing.obj
echo "Exit code: $?"
echo

echo "Test 5: Watch stub (should fail with code 4)"
$GLINT watch
echo "Exit code: $?"
echo

echo "==================================="
echo "Quick test complete!"
echo "==================================="
```

---

## Troubleshooting

### Issue: "glint.exe is not recognized"

**Solution**: Use full path or add to PATH:
```batch
# Windows
set PATH=%PATH%;D:\ahoqp1\Repositories\Glint3D\builds\desktop\cmake\Debug

# Then you can just use:
glint.exe clean --dry-run
```

### Issue: Commands hang or crash

**Check**:
1. Build completed successfully (no linker errors)
2. All DLLs are present in the build directory
3. Working directory is correct (repo root for asset paths)

### Issue: No output from commands

**Check**:
1. Verbosity level (try `--verbosity debug`)
2. stderr vs stdout (some output goes to stderr)
3. Redirect both: `glint.exe doctor 2>&1`

### Issue: FileNotFound errors for assets

**Solution**: Run from repo root:
```bash
cd D:\ahoqp1\Repositories\Glint3D
.\builds\desktop\cmake\Debug\glint.exe <command>
```

---

## Expected Test Results Summary

| Test Suite | Expected Pass Rate | Notes |
|------------|-------------------|-------|
| Basic Commands | 100% | All verbs should be recognized |
| Clean Command | 100% | Fully implemented |
| Inspect Command | 80% | Works for files, engine integration partial |
| Validate Command | 70% | Works if manifests exist |
| Config Command | 100% | Should always run |
| Doctor Command | 100% | Should always run |
| Modules Command | 100% | Fully implemented |
| Init Command | 100% | Fully implemented |
| Render Command | 50% | Placeholder logic, manifests work |
| Exit Codes | 100% | All codes should be correct |
| Global Flags | 90% | Should work if dispatcher wired |

---

## Next Steps After Testing

1. **Document any bugs** in a `BUGS.md` file
2. **Create GitHub issues** for failures
3. **Update test coverage** based on results
4. **Automate passing tests** in CI

---

*Happy Testing!* 🧪
