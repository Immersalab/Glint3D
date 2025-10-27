# Task Module System - Developer Guide

**This is the workflow we use to develop Glint3D.** It helps humans and AI work together on tasks in a clear, organized way. The system keeps AI agents controlled and context-aware, so they know exactly what to do and contribute clean, quality code without getting lost or over-engineering.

## What is a Task Module?

A **task module** is a structured folder that holds everything needed to plan, do, track, and check a specific engineering task. 
It gives both humans and AI a clear, shared way to understand and work on the same task.

Think of a task module as a **“work capsule”** that includes:  
- **Specification**: What needs to be done and why  
- **Execution plan**: Clear steps to follow  
- **Progress tracking**: Real-time updates on what’s finished  
- **Validation criteria**: How to confirm the task is complete  
- **Artifacts**: The outputs and documentation created along the way  

## Why Use Task Modules?

### Benefits for Developers
- **Clear scope**: Each task has defined inputs, outputs, and success criteria  
- **Progress visibility**: You can track updates in real time  
- **Context preservation**: All task information stays in one place  
- **Easy handoff**: Anyone can pick up where the last person left off  
- **Audit trail**: A full history of what was done and when  

### Benefits for AI Collaboration
- **Predictable execution**: AI knows exactly what steps to take  
- **Quality checks**: Acceptance criteria prevent unfinished work  
- **Context loading**: AI can resume work without losing track  
- **Fewer errors**: A clear structure reduces guesswork and confusion

## Directory Structure

Each task module follows this canonical structure:

```
ai/tasks/{task_name}/
├── task.json              # Complete task specification (REQUIRED)
├── checklist.md           # Atomic execution steps (REQUIRED)
├── progress.ndjson        # Event tracking history (REQUIRED)
├── artifacts/             # Generated outputs (REQUIRED)
│   ├── README.md          # Description of expected artifacts
│   ├── code/              # Generated code files
│   ├── tests/             # Test files and results
│   ├── documentation/     # Generated documentation
│   └── validation/        # Validation results and reports
└── [optional files]       # Task-specific files (coverage.md, mapping.md, etc.)
```

## Core Files Explained

### 1. task.json - Task Specification

The `task.json` file is the single source of truth for what needs to be accomplished.  
It contains all the essential information about a task in a clean, structured, and machine-readable format.

We use **JSON** because:  
- It’s easy for both humans and AI to read and update.  
- It provides a clear, standardized structure for storing task details.  
- AI systems can quickly parse and act on the information without guesswork.  
- It ensures consistent formatting across all tasks, reducing context loss.  
- It allows automation, making it simple to link tasks, track progress, and enforce requirements.

**Schema:**
```json
{
  "id": "unique_task_identifier",
  "title": "Human Readable Task Title",
  "owner": "responsible_team",
  "status": "pending|in_progress|completed|blocked",
  "inputs": {
    "files": ["list/of/input/files.cpp", "with/line/ranges.h:100-200"],
    "schema_version": "required_schema_version"
  },
  "outputs": {
    "artifacts": ["expected/output/files", "documentation.md"],
    "events": ["telemetry_event_types", "logged_to_progress_ndjson"]
  },
  "acceptance": [
    "Specific acceptance criterion 1",
    "Specific acceptance criterion 2",
    "Agent-testable requirement 3"
  ],
  "safety": {
    "rate_limits": ["max_build_time_10min", "max_file_size_1mb"],
    "sandbox": true
  },
  "dependencies": ["prerequisite_task_1", "prerequisite_task_2"],
  "seed_policy": "deterministic|non_deterministic"
}
```

**Key Fields:**  
- **id**: A unique name that matches the task’s folder.  
- **status**: Shows where the task is right now (pending, in progress, completed, or blocked).  
- **inputs.files**: The files and line ranges that will be changed.  
- **outputs.artifacts**: The expected results or deliverables.  
- **acceptance**: The rules or checks that must be met for the task to be finished.  
- **dependencies**: Other tasks that need to be done first.

### 2. checklist.md - Execution Plan

The checklist is a simple list of clear steps that break the task into small, easy-to-finish actions.  
Each step should be small enough to finish in one work session (around 15–60 minutes).

**Format:**
```markdown
# Task Name Checklist

## Phase 1: Setup & Analysis
- [ ] **Read existing implementation** - Review current code in engine/src/foo.cpp
- [ ] **Identify dependencies** - Document what systems are affected
- [x] **Create backup branch** - git checkout -b task/feature-name

## Phase 2: Implementation
- [ ] **Migrate function X** - Convert foo() to use RHI API
- [ ] **Add unit tests** - Test foo() with valid/invalid inputs
- [ ] **Update documentation** - Document new API in header comments

## Phase 3: Validation
- [ ] **Build verification** - cmake --build builds/desktop/cmake -j
- [ ] **Run tests** - ./run_tests.sh
- [ ] **Visual regression** - Compare golden images
```

**Best Practices:**
- Use `[x]` to mark completed items
- Include **bold task names** for easy scanning
- Provide specific commands or file paths where relevant
- Group related steps into logical phases
- Keep steps atomic (one clear action per checkbox)

### 3. progress.ndjson - Event Log

The `progress.ndjson` file is a simple event log that records everything that happens during a task.  
It uses **newline-delimited JSON**, which means each line is one complete JSON object.  
This makes it easy for both humans and AI to read, write, and process events in order, without needing a big or complex file format.

We use this format because:  
- It’s easy to append new events as they happen.  
- AI can quickly read and understand the task’s history without extra parsing.  
- It keeps a clean, ordered timeline of what was done.  
- It makes resuming, auditing, and debugging tasks much simpler.

**Format:**
```json
{"ts": "2024-12-28T00:00:00Z", "task_id": "task_name", "event": "task_started", "phase": "initial", "status": "in_progress", "agent": "human"}
{"ts": "2024-12-28T01:30:00Z", "task_id": "task_name", "event": "step_completed", "step": "Read existing implementation", "status": "success", "details": "Reviewed 250 lines", "agent": "claude"}
{"ts": "2024-12-28T02:15:00Z", "task_id": "task_name", "event": "build_failed", "error": "undefined reference to RHI::clear()", "agent": "claude"}
{"ts": "2024-12-28T02:45:00Z", "task_id": "task_name", "event": "phase_completed", "phase": "implementation", "next_phase": "validation", "agent": "claude"}
```

**Standard Event Types:**
- `task_started` - Task begins
- `step_completed` - Checklist item finished
- `phase_completed` - Major section done
- `build_succeeded` / `build_failed` - Compilation results
- `test_passed` / `test_failed` - Test results
- `blocker_encountered` - Something prevents progress
- `task_completed` - All acceptance criteria met

**Required Fields:**
- `ts`: ISO 8601 timestamp
- `task_id`: Matches directory name
- `event`: Event type from standard set
- `agent`: Who/what performed the action (human, claude, github_ci, etc.)

**Optional Fields:**
- `status`: success, failure, warning
- `details`: Human-readable description
- `step`: References specific checklist item
- `phase`: Current phase name
- `error`: Error message if applicable

### 4. artifacts/ - Generated Outputs

The artifacts directory contains all deliverables produced during task execution.

**Standard Subdirectories:**
- `code/` - Generated or modified code files
- `tests/` - Test files and test results
- `documentation/` - Generated docs, API references, migration guides
- `validation/` - Performance reports, coverage reports, benchmark results

**artifacts/README.md** should list:
- Expected artifacts and their purposes
- Naming conventions
- Validation requirements

## How to Prompt AI to Use Task Modules

### Starting a New Task

```
I want to work on the "pass_bridging" task.
Please follow and read thorugh the ai/FOR_MACHINES.md protocol.
```

The FOR_MACHINES.md document is the universal AI execution protocol. It will:
1. Read ai/tasks/current_index.json to confirm the active task
2. Load all task context files
3. Execute checklist steps sequentially
4. Update progress.ndjson after each step
5. Generate all required artifacts
6. Validate acceptance criteria before completion

### Resuming Work on an Existing Task

```
Continue working on the current task.
Please follow the ai/FOR_MACHINES.md protocol.
```

The protocol will automatically determine where you left off by reading progress.ndjson and continue from the next incomplete step.

### Checking Task Status

```
What's the status of the current task?
Please read ai/tasks/current_index.json and progress.ndjson to summarize current state.
```

### Switching Tasks

```
I need to switch to "new_task_name" because of a blocker.
Please follow the ai/FOR_MACHINES.md blocker handling protocol.
```

The FOR_MACHINES.md protocol includes proper blocker handling:
- Logs blocker_encountered event
- Updates current_index.json with blocker details
- Switches to the new task if instructed
- Maintains proper event history

## How to Create a New Task Module

### Using the Task Creation Prompt (Recommended)

The easiest way to create a new task is to describe what you want and let AI format it properly.

**Step 1:** Describe your task in plain language to any AI (like ChatGPT):
```
I need to add a bloom post-processing effect to the renderer.
It should modify the lighting shader and add a new blur pass.
```

**Step 2:** Ask the AI to format it for the task creation prompt:
```
Format this task for the Glint3D task creation system with proper inputs, outputs, and acceptance criteria.
```

**Step 3:** Use the formatted details with the task creation prompt:
```
Create a new task module using ai/prompts/TASK_MODULE_CREATION_PROMPT.md with:
- ID: add_bloom_effect
- Title: Add Bloom Post-Processing Effect
- Owner: rendering_team
- Inputs: ["engine/src/lighting.cpp", "engine/shaders/post_process.glsl"]
- Outputs: ["artifacts/code/bloom_pass.cpp", "artifacts/tests/bloom_test.cpp"]
- Acceptance criteria:
  * Bloom effect renders correctly
  * Performance impact under 2ms
  * All tests pass
  * Documentation updated
- Dependencies: ["lighting_system"]
```

The task creation prompt will automatically:
1. Create the full folder structure (ai/tasks/add_bloom_effect/)
2. Generate task.json with all the details
3. Create checklist.md with clear steps
4. Set up progress.ndjson for tracking
5. Create artifacts/README.md
6. Register the task in current_index.json
7. Validate everything is correct

### Manual Creation (Advanced)

If you prefer to create a task module manually, follow the canonical structure:

1. **Create directory**: `ai/tasks/{task_id}/`
2. **Create required files**: task.json, checklist.md, progress.ndjson
3. **Create artifacts structure**: `artifacts/{code,tests,documentation,validation}/`
4. **Create artifacts/README.md**: Document expected outputs
5. **Register in current_index.json**: Add to task_status section

See the file format specifications in ai/FOR_MACHINES.md for exact schemas.

## How Task Modules Work

### Task Lifecycle

```
pending → ready → in_progress → completed
                       ↓
                    blocked (if issues arise)
                       ↓
                  in_progress (once unblocked)
```

### Execution Flow

1. **Task Selection**: AI reads `current_index.json` to identify the active task
2. **Context Loading**: AI reads `task.json`, `checklist.md`, and `progress.ndjson`
3. **Step Execution**: AI works through checklist items sequentially
4. **Progress Logging**: After each step, AI appends an event to `progress.ndjson`
5. **Artifact Generation**: AI creates outputs in `artifacts/` directory
6. **Validation**: AI checks acceptance criteria from `task.json`
7. **Completion**: AI marks task as complete and updates `current_index.json`

### Dependency Management

Tasks can depend on other tasks. The system automatically:
- Blocks tasks until dependencies are met
- Updates `ready_when` conditions in `current_index.json`
- Maintains a `critical_path` array showing the sequence of tasks

Example:
```json
{
  "critical_path": [
    "opengl_migration",
    "pass_bridging",
    "state_accessibility",
    "ndjson_telemetry"
  ]
}
```

### CI/CD Integration (Future)

Eventually, we can use **Codex for GitHub Actions** to automate task validation. If you don't know what that is, you should look into it—it's a game changer for structured workflows like this.

Because our task modules are deterministic and machine-readable, AI agents can run in CI to:
- Validate `task.json` has required fields and proper format
- Check `checklist.md` has all steps completed before merge
- Verify `progress.ndjson` has proper event sequence (task_started → step_completed → task_completed)
- Confirm all `outputs.artifacts` files exist
- Run acceptance criteria checks automatically
- Execute QA passes using `ai/prompts/QA_PASS_PROMPT.md`
- Generate findings and reports in `ai/qa/`

This means every PR can be automatically validated against the task specification, ensuring no incomplete work gets merged.

## AI Prompts and QA Resources

### Prompts Directory (`ai/prompts/`)

The prompts directory contains specialized AI prompts for task management and quality assurance:

#### QA_PASS_PROMPT.md
A structured prompt for conducting architecture quality assurance passes. Use this to have AI agents:
- Analyze system architecture and organization
- Identify modularity, navigability, and controllability issues
- Generate structured QA reports in `ai/qa/`
- Create prioritized findings with remediation recommendations
- Ensure alignment with architecture vision

**Usage:**
```
Please perform a QA pass following the ai/prompts/QA_PASS_PROMPT.md specification.
Focus on [specific subsystem or concern].
```

#### TASK_MODULE_CREATION_PROMPT.md
A deterministic protocol for creating new task modules. Use this to have AI agents:
- Generate complete task module structure
- Create all required files (task.json, checklist.md, progress.ndjson)
- Register tasks in current_index.json
- Set up artifact directories

**Usage:**
```
Create a new task module using ai/prompts/TASK_MODULE_CREATION_PROMPT.md with:
- ID: my_new_task
- Title: Implement Feature X
- Inputs: [list of files]
- Outputs: [expected artifacts]
- Acceptance criteria: [list]
```

### QA Directory (`ai/qa/`)

The QA directory stores quality assurance findings and reports generated during QA passes:

**Structure:**
```
ai/qa/
├── progress.ndjson              # QA event history
├── findings/
│   ├── open/                    # Active issues
│   │   ├── QA_001_issue.json    # Individual issue files
│   │   └── QA_002_issue.json
│   └── resolved/                # Fixed issues
├── reports/
│   ├── index.json               # Report registry
│   ├── QA_2024-12-28_qa_pass.json          # Structured report
│   └── QA_2024-12-28_qa_summary.md         # Human-readable summary
```

**QA Findings Format:**
Each finding includes:
- Unique ID
- Priority (High, Medium, Low)
- Impact assessment
- Affected component
- Description of the issue
- Recommended remediation
- Links to related task modules (if applicable)

**Usage:**
```
Please perform a QA pass following the ai/prompts/QA_PASS_PROMPT.md specification.
```

## Documentation Standards

### Doxygen API Documentation

The project uses **Doxygen** to generate API documentation. All public headers in `engine/include/` must use Doxygen-compliant comments with `@file`, `@brief`, `@param`, `@return`, etc.

**Generate documentation:**
```bash
# Windows
tools\generate-docs.bat

# Linux/macOS
./tools/generate-docs.sh
```

Output: `docs/api/html/index.html`

**Important:** Per FOR_MACHINES.md §0C, tasks cannot be marked complete if header documentation is missing or outdated. AI agents must verify documentation after modifying any header files.

## Next Steps

- Read `ai/FOR_MACHINES.md` for the AI execution protocol
- Review existing task modules in `ai/tasks/` for examples
- Generate API documentation with the scripts in `docs/`