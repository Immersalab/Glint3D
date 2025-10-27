# AI TASK MODULE CREATION CONTRACT

## 1. PURPOSE
This contract defines how the AI MUST create a new task module in the Glint3D task system.
The goal is to generate the required structure and files **deterministically** based only on the information provided.

## 2. CORE RULES

AI MUST:
- Use only provided task details (id, title, inputs, outputs, acceptance, dependencies).
- Create the canonical folder and file structure exactly.
- Fill required files with valid content, no placeholders.
- Append initial events to progress.ndjson in chronological order.
- Register the task in current_index.json as `pending`.

AI MUST NOT:
- Hallucinate fields, steps, dependencies, or artifacts.
- Create optional files unless explicitly instructed.
- Skip required files or steps.

If required information is missing → STOP and report.

## 3. DIRECTORY STRUCTURE

```
ai/tasks/{task_id}/
├── task.json
├── checklist.md
├── progress.ndjson
├── artifacts/
│   ├── README.md
│   ├── code/
│   ├── tests/
│   ├── documentation/
│   └── validation/
```

## 4. CREATION STEPS (ORDER MATTERS)

1. **Create directory** `ai/tasks/{task_id}/` with full structure.
2. **Generate task.json** using provided info:
   - status must be `"pending"`
   - events must include at least `"task_started"` and `"task_completed"`
3. **Generate checklist.md** with at least one phase and atomic steps from user input.
4. **Create progress.ndjson** with initial event:
   ```json
   {"ts":"{ISO_8601}","task_id":"{task_id}","event":"task_created","status":"pending","agent":"{agent}"}
   ```
5. **Generate artifacts/README.md** using standard template.
6. **Register in current_index.json**:
   - task_status[{task_id}].status = "pending"
   - blocked_by = []
   - priority = "medium" (default if not specified)
7. **Append task_registered event** to progress.ndjson.
8. REPORT: `Task module {task_id} created and registered successfully.`

## 5. LOGGING

Required events in progress.ndjson:
- `task_created` (after file creation)
- `task_registered` (after index update)

Each event must include:
- ts (ISO 8601)
- task_id
- event
- agent

## 6. VALIDATION

Before reporting success:
- Confirm all required files exist and are non-empty.
- Confirm task.json parses as valid JSON.
- Confirm current_index.json entry matches task_id.
- Confirm at least `task_created` and `task_registered` events are logged.

## 7. ANTI-HALLUCINATION

- If a field is missing, prompt for it or stop.
- Do not make assumptions or generate default acceptance criteria, dependencies, or artifacts.
- Do not create extra files not part of the structure.

# END OF SPEC v1.1
