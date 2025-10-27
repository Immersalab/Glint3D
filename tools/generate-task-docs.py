#!/usr/bin/env python3
"""
Auto-generate task module documentation from ai/tasks/ directory.
Reads current_index.json, task.json, checklist.md, and README.md from each task.
Outputs docs/TASKS.md for inclusion in Doxygen documentation.
"""

import json
import os
import re
from pathlib import Path
from datetime import datetime

def read_json_file(filepath):
    """Read and parse a JSON file."""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            return json.load(f)
    except Exception as e:
        print(f"Warning: Could not read {filepath}: {e}")
        return None

def read_text_file(filepath):
    """Read a text file."""
    try:
        with open(filepath, 'r', encoding='utf-8') as f:
            return f.read()
    except Exception as e:
        print(f"Warning: Could not read {filepath}: {e}")
        return None

def count_checklist_items(checklist_content):
    """Count completed and total checklist items."""
    if not checklist_content:
        return 0, 0

    completed = len(re.findall(r'- \[x\]', checklist_content, re.IGNORECASE))
    total = len(re.findall(r'- \[[x ]\]', checklist_content, re.IGNORECASE))

    return completed, total

def get_task_priority_emoji(priority):
    """Get emoji for task priority."""
    priority_map = {
        'critical': '🔴',
        'high': '🟠',
        'medium': '🟡',
        'low': '🟢'
    }
    return priority_map.get(priority.lower(), '⚪')

def get_status_emoji(status):
    """Get emoji for task status."""
    status_map = {
        'completed': '✅',
        'in_progress': '🔄',
        'blocked': '🚫',
        'pending': '⏳'
    }
    return status_map.get(status.lower(), '⚪')

def generate_placeholder_tasks_md():
    """Generate placeholder TASKS.md when ai/tasks directory doesn't exist."""
    lines = []
    lines.append('# Task Modules {#tasks}')
    lines.append('')
    lines.append('This page provides an overview of the AI-driven task module system used to manage Glint3D development.')
    lines.append('')
    lines.append('> **Note**: Task system not currently configured for this repository.')
    lines.append('')
    lines.append('## Overview {#tasks_overview}')
    lines.append('')
    lines.append('Glint3D can use a structured task capsule system located in `ai/tasks/` for systematic development.')
    lines.append('')
    lines.append('Each task module can contain:')
    lines.append('')
    lines.append('- **task.json** - Task metadata, inputs, outputs, acceptance criteria')
    lines.append('- **checklist.md** - Phase-based checklist of implementation steps')
    lines.append('- **progress.ndjson** - Event log tracking execution history')
    lines.append('- **README.md** - Task overview and context')
    lines.append('- **artifacts/** - Generated documentation and reports')
    lines.append('')
    lines.append('## Configuration')
    lines.append('')
    lines.append('To enable the task system:')
    lines.append('')
    lines.append('1. Create `ai/tasks/` directory')
    lines.append('2. Add `current_index.json` with task definitions')
    lines.append('3. Run `python tools/generate-task-docs.py` to generate documentation')
    lines.append('')
    lines.append('See the AI development protocols documentation for more information.')
    lines.append('')
    lines.append('---')
    lines.append('')
    lines.append(f"**Auto-generated**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}  ")
    lines.append('')

    return '\n'.join(lines)

def generate_task_section(task_id, task_status, task_dir):
    """Generate markdown section for a single task."""
    task_json_path = task_dir / 'task.json'
    checklist_path = task_dir / 'checklist.md'
    readme_path = task_dir / 'README.md'

    # Read task data
    task_json = read_json_file(task_json_path)
    checklist_content = read_text_file(checklist_path)
    readme_content = read_text_file(readme_path)

    # Extract information
    status = task_status.get('status', 'unknown')
    priority = task_status.get('priority', 'medium')

    # Build section
    lines = []
    lines.append(f"#### {task_id}")

    # Status line
    status_parts = [f"**Status**: {status.title()}"]

    if status == 'blocked':
        blocked_by = task_status.get('blocked_by', [])
        if blocked_by:
            status_parts.append(f"**Blocked By**: {', '.join(blocked_by)}")

    if 'completion_percentage' in task_status:
        status_parts.append(f"**Progress**: {task_status['completion_percentage']}%")

    if 'phase' in task_status or 'current_phase' in task_status:
        phase = task_status.get('phase') or task_status.get('current_phase')
        status_parts.append(f"**Phase**: {phase}")

    if priority:
        status_parts.append(f"**Priority**: {priority.title()}")

    lines.append(' | '.join(status_parts))
    lines.append('')

    # Description from task.json or README
    if task_json and 'title' in task_json:
        lines.append(task_json['title'])
        lines.append('')
    elif readme_content:
        # Extract first paragraph from README
        readme_lines = readme_content.split('\n')
        for line in readme_lines[1:]:  # Skip title
            if line.strip() and not line.startswith('#'):
                lines.append(line.strip())
                lines.append('')
                break

    # Checklist progress
    if checklist_content:
        completed, total = count_checklist_items(checklist_content)
        if total > 0:
            percentage = int((completed / total) * 100)
            lines.append(f"**Checklist**: {completed}/{total} items complete ({percentage}%)")
            lines.append('')

    # Acceptance criteria count
    if task_json and 'acceptance' in task_json:
        criteria_count = len(task_json['acceptance'])
        lines.append(f"**Acceptance Criteria**: {criteria_count} criteria defined")
        lines.append('')

    # Link to task directory
    lines.append(f"**Location**: `ai/tasks/{task_id}/`")
    lines.append('')
    lines.append('---')
    lines.append('')

    return '\n'.join(lines)

def generate_tasks_md(repo_root):
    """Generate complete TASKS.md file."""
    ai_dir = repo_root / 'ai'
    tasks_dir = ai_dir / 'tasks'
    current_index_path = tasks_dir / 'current_index.json'

    # Check if ai/tasks directory exists
    if not tasks_dir.exists():
        print("Info: ai/tasks directory not found - generating placeholder documentation")
        return generate_placeholder_tasks_md()

    # Read current index
    current_index = read_json_file(current_index_path)
    if not current_index:
        print("Warning: Could not read current_index.json - generating placeholder documentation")
        return generate_placeholder_tasks_md()

    task_status = current_index.get('task_status', {})
    critical_path = current_index.get('critical_path', [])
    current_focus = current_index.get('current_focus', {})

    # Start building document
    lines = []
    lines.append('# Task Modules {#tasks}')
    lines.append('')
    lines.append('This page provides an overview of the AI-driven task module system used to manage Glint3D development.')
    lines.append('')
    lines.append('> **Note**: This documentation is auto-generated from `ai/tasks/` - do not edit manually!')
    lines.append('')

    # Overview section
    lines.append('## Overview {#tasks_overview}')
    lines.append('')
    lines.append('Glint3D uses a structured task capsule system located in `ai/tasks/` for systematic development. Each task module contains:')
    lines.append('')
    lines.append('- **task.json** - Task metadata, inputs, outputs, acceptance criteria')
    lines.append('- **checklist.md** - Phase-based checklist of implementation steps')
    lines.append('- **progress.ndjson** - Event log tracking execution history')
    lines.append('- **README.md** - Task overview and context (when applicable)')
    lines.append('- **artifacts/** - Generated documentation and reports')
    lines.append('')

    # Current focus
    if current_focus:
        lines.append('## Current Focus {#tasks_current_focus}')
        lines.append('')
        active_task = current_focus.get('active_task')
        if active_task:
            lines.append(f"**Active Task**: `{active_task}`  ")
            if 'phase' in current_focus:
                lines.append(f"**Phase**: {current_focus['phase']}  ")
            if 'notes' in current_focus:
                lines.append(f"**Notes**: {current_focus['notes']}  ")
        lines.append('')

    # Critical path
    if critical_path:
        lines.append('## Critical Path {#tasks_critical_path}')
        lines.append('')
        lines.append('The following tasks form the critical dependency chain:')
        lines.append('')
        lines.append('```')
        lines.append(' → '.join(critical_path))
        lines.append('```')
        lines.append('')

    # Group tasks by status
    completed_tasks = []
    in_progress_tasks = []
    blocked_tasks = []
    pending_tasks = []

    for task_id, status in task_status.items():
        task_status_val = status.get('status', 'unknown')
        if task_status_val == 'completed':
            completed_tasks.append(task_id)
        elif task_status_val == 'in_progress':
            in_progress_tasks.append(task_id)
        elif task_status_val == 'blocked':
            blocked_tasks.append(task_id)
        elif task_status_val == 'pending':
            pending_tasks.append(task_id)

    # In progress tasks
    if in_progress_tasks:
        lines.append('## 🔄 In Progress {#tasks_in_progress}')
        lines.append('')
        for task_id in in_progress_tasks:
            task_dir = tasks_dir / task_id
            if task_dir.exists():
                lines.append(generate_task_section(task_id, task_status[task_id], task_dir))

    # Blocked tasks
    if blocked_tasks:
        lines.append('## 🚫 Blocked Tasks {#tasks_blocked}')
        lines.append('')
        for task_id in blocked_tasks:
            task_dir = tasks_dir / task_id
            if task_dir.exists():
                lines.append(generate_task_section(task_id, task_status[task_id], task_dir))

    # Pending tasks
    if pending_tasks:
        lines.append('## ⏳ Pending Tasks {#tasks_pending}')
        lines.append('')
        for task_id in pending_tasks:
            task_dir = tasks_dir / task_id
            if task_dir.exists():
                lines.append(generate_task_section(task_id, task_status[task_id], task_dir))

    # Completed tasks
    if completed_tasks:
        lines.append('## ✅ Completed Tasks {#tasks_completed}')
        lines.append('')
        for task_id in completed_tasks:
            task_dir = tasks_dir / task_id
            if task_dir.exists():
                lines.append(generate_task_section(task_id, task_status[task_id], task_dir))

    # Footer
    lines.append('---')
    lines.append('')
    lines.append('## Task System Protocol {#tasks_protocol}')
    lines.append('')
    lines.append('All tasks follow the **FOR_MACHINES.md** protocol for deterministic AI-driven execution.')
    lines.append('')
    lines.append('See: `ai/FOR_MACHINES.md`')
    lines.append('')
    lines.append('---')
    lines.append('')
    lines.append(f"**Auto-generated**: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}  ")
    lines.append(f"**Total Tasks**: {len(task_status)}  ")
    lines.append(f"**Source**: `ai/tasks/current_index.json`  ")
    lines.append('')

    return '\n'.join(lines)

def main():
    """Main entry point."""
    # Find repository root
    script_dir = Path(__file__).parent.absolute()
    repo_root = script_dir.parent

    print(f"Repository root: {repo_root}")
    print("Generating task documentation...")

    # Generate documentation
    tasks_md = generate_tasks_md(repo_root)

    if tasks_md:
        # Write to docs/TASKS.md
        output_path = repo_root / 'docs' / 'TASKS.md'
        output_path.parent.mkdir(parents=True, exist_ok=True)

        with open(output_path, 'w', encoding='utf-8') as f:
            f.write(tasks_md)

        print(f"[OK] Generated: {output_path}")
        print(f"  Lines: {len(tasks_md.splitlines())}")
    else:
        print("[FAIL] Failed to generate documentation")
        return 1

    return 0

if __name__ == '__main__':
    exit(main())