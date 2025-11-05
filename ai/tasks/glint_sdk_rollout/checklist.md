<!-- Machine Summary Block -->
{"file":"ai/tasks/glint_sdk_rollout/checklist.md","purpose":"Tracks rollout plan for Glint SDKs (C++, Python, Node).","exports":[],"depends_on":["ai/tasks/glint_sdk_rollout/task.json"],"notes":["sdk_milestone"]}
<!-- Human Summary -->
Checklist for delivering stable SDKs that mirror CLI/RPC capabilities with reproducible behavior.

# Glint SDK Rollout Checklist

## Phase 1: Scope & Contracts
- [ ] Define SDK personas, support matrix (languages, versions), and distribution channels.
- [ ] Specify API surfaces for C++ core, Python, and Node packages (mirroring CLI/RPC semantics).
- [ ] Document error handling, async patterns, and determinism expectations.
- [ ] Establish versioning + compatibility story (semver ranges, schema pinning).

## Phase 2: Implementation
- [ ] Expose core rendering/project APIs in C++ with module registration hooks.
- [ ] Build Python SDK (`glint`) with Project/render/validate/profile helpers and run manifest access.
- [ ] Ship Node SDK (`@glint/sdk`) with job orchestration, file watching, and template utilities.
- [ ] Integrate SDKs with CLI/RPC (shared contracts, codegen for request/response types).
- [ ] Provide stub/test harnesses for future language bindings.

## Phase 3: Validation & Distribution
- [ ] Create SDK reference docs + examples (per language) and quickstart notebooks/projects.
- [ ] Publish CI pipelines for packaging (PyPI, npm, vcpkg/Conan) with signed artifacts.
- [ ] Assemble conformance tests ensuring parity across CLI/RPC/SDK.
- [ ] Capture compatibility matrix (engine ↔ schema ↔ SDK versions).
- [ ] Launch developer preview announcement + feedback loop.
