# QA PASS PROMPT CONTRACT

## Objective
You are an AI agent performing a QA PASS on the Glint3D architecture.  
The purpose of this pass is to identify architectural, structural, and organizational issues that may impact:
- Software engineering quality
- Agent navigability and controllability
- Modular and scalable system design
- Alignment with the architecture vision defined in `docs/GOD_DOCUMENT`.

## Core Rules

1. You MUST ground all findings in the actual project structure and code.  
2. You MUST NOT hallucinate issues or architecture directions. If information is missing, log as `needs_clarification`.  
3. All findings and reports MUST be stored in `ai/tasks/qa/` according to the QA folder spec.  
4. All events MUST be logged in `ai/tasks/qa/progress.ndjson`.  
5. All issues MUST be stored in `ai/tasks/qa/findings/open/` with unique IDs.  
6. You MUST generate a structured report in `ai/tasks/qa/reports/` and update `ai/tasks/qa/reports/index.json`.  
7. You MUST align all recommendations with the principles defined in `docs/god_document`.  
8. You MUST prioritize findings based on impact (Modularity, Navigability, Controllability, Simplicity, Vision Alignment).  
9. You MUST explicitly log critical findings and link them to remediation task modules when available.  
10. You MUST NOT produce stylistic-only recommendations without clear impact.

## QA PASS EXECUTION STEPS

1. **Context Load**
   - Load relevant architectural structure, docs, and god_document.

2. **System Analysis**
   - Identify modules, subsystems, and dependency relationships.
   - Check for coupling, cohesion, navigability, and alignment with god_document.

3. **Evaluate Against Principles**
   - Readability
   - Navigability
   - Controllability
   - Modularity
   - Simplicity
   - Traceability
   - Vision alignment

4. **Issue Identification**
   - Assign each issue:
     - Unique ID
     - Priority (High, Medium, Low)
     - Impact
     - Component
     - Description
     - Recommendation
   - Store issues in `ai/tasks/qa/findings/open/`.

5. **Report Generation**
   - Create a structured JSON report in `ai/tasks/qa/reports/`.
   - Create a Markdown summary.
   - Update `ai/tasks/qa/reports/index.json`.

6. **Logging**
   - Append `qa_pass_started` and `qa_pass_completed` events in `ai/tasks/qa/progress.ndjson`.
   - Append `qa_issue_created` for each issue.

7. **Finalization**
   - Ensure the structured report, issues, and logs are consistent.
   - No missing artifacts.
   - No hallucinated content.
   - All issues are grounded in real observations.

## Output Requirements

- `ai/tasks/qa/reports/QA_<date>_qa_pass.json` (structured)
- `ai/tasks/qa/reports/QA_<date>_qa_summary.md` (human-readable)
- `ai/tasks/qa/findings/open/*` (per-issue files)
- Updated `ai/tasks/qa/reports/index.json`
- Appended events in `ai/tasks/qa/progress.ndjson`

## Anti-Hallucination and Control

- If data is missing or ambiguous, STOP and log `needs_clarification`.
- Do not fabricate issues, recommendations, or directions.
- Do not deviate from the architecture vision in `docs/god_document`.
- Recommendations must be technically justifiable.

# END OF QA PROMPT CONTRACT v1.0
