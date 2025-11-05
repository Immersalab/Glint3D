<!-- Machine Summary Block -->
{"file":"ai/tasks/glint_rpc_daemon/checklist.md","purpose":"Tracks milestones for the Glint RPC daemon and JSON-RPC surface.","exports":[],"depends_on":["ai/tasks/glint_rpc_daemon/task.json"],"notes":["milestone_b_rpc"]}
<!-- Human Summary -->
Checklist covering service design, transport, API methods, and validation for the long-running RPC daemon.

# Glint RPC Daemon Checklist

## Phase 1: Design & Contract
- [ ] Draft RPC product spec (use cases, lifecycle, security model, transport selection).
- [ ] Define JSON-RPC method catalog, params/results schemas, and error codes.
- [ ] Document event stream contract (job.started, job.progress, job.completed, job.failed) and delivery guarantees.
- [ ] Plan authentication/authorization (local only vs token) and resource isolation.

## Phase 2: Implementation
- [ ] Implement `glint agent start` / `glintd` lifecycle with graceful shutdown and restart.
- [ ] Wire scene.validate, render.start/status/cancel, assets.sync, profile.run, and health endpoints.
- [ ] Integrate run manifest + determinism logging into RPC job orchestration.
- [ ] Expose streaming progress/events over HTTP(S) or named pipe + SSE/WebSocket fallback.
- [ ] Provide structured logging + tracing hooks for downstream observability.

## Phase 3: Validation & Ops
- [ ] Build contract tests (protocol-level) and integration suite with CLI + SDK clients.
- [ ] Publish service reference docs and quickstart (including machine-readable OpenRPC spec).
- [ ] Capture performance + stability validation (concurrency, cancellation, large assets).
- [ ] Harden deployment story (service install, user permissions, auto-update plan).
- [ ] Align roadmap with SDK releases and note dependencies in current_index.
