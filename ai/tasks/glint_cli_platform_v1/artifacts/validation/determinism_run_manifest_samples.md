<!-- Machine Summary Block -->
{"file":"ai/tasks/glint_cli_platform_v1/artifacts/validation/determinism_run_manifest_samples.md","purpose":"Outlines determinism validation hooks, run manifest payloads, and sample NDJSON traces for Glint CLI renders.","exports":["run_manifest_structure","ndjson_trace_samples","validation_hooks"],"depends_on":["ai/tasks/glint_cli_platform_v1/artifacts/documentation/glint_project_manifest_and_run_manifest_spec.md","ai/tasks/glint_cli_platform_v1/artifacts/code/cli_architecture_overview.md"],"notes":["determinism_contract","render_command_hooks","smoke_suite_alignment"]}
<!-- Human Summary -->
Plan and examples for validating deterministic render runs, including run manifest payload fields and NDJSON traces wired through CLI commands.

# Determinism Validation Hooks & Samples

## 1. Hook Diagram
- `RenderCommand` will orchestrate the pipeline:
  1. Resolve manifest + configuration (via `ConfigResolver`).
  2. Prepare render job metadata (scene, device, frame set, RNG seed).
  3. Invoke engine render entry.
  4. Delegate provenance capture to `RunManifestWriter`.
  5. Stream NDJSON events (`render_started`, `render_frame_completed`, `run_manifest_written`, `render_completed`).
- `ValidateCommand` reuses `RunManifestWriter` in dry mode to confirm hashes and schema compatibility without rendering.
- `DoctorCommand` leverages the same hooks to confirm determinism assets (module digests, schema cache) are aligned.

## 2. Run Manifest Payload Skeleton
```json
{
  "schema_version": "1.0.0",
  "run_id": "f3c88a38-6f23-47fc-9ab9-2215df7f1ac6",
  "timestamp_utc": "2025-11-05T23:30:12Z",
  "cli": {
    "command": "render",
    "arguments": ["--project", "shows/demo/glint.project.json", "--scene", "shots/010.json"],
    "exit_code": 0,
    "json_mode": true
  },
  "platform": {
    "os": "Windows 11",
    "cpu": "12th Gen Intel(R) Core(TM) i9-12900K",
    "gpu": "NVIDIA RTX 4090",
    "driver_version": "546.17",
    "kernel": "10.0.22635"
  },
  "engine": {
    "version": "0.3.0",
    "modules": [
      {"name": "core", "version": "3.0.0", "hash": "6f9c..."},
      {"name": "raytracing", "version": "2.1.0", "hash": "ab31..."}
    ],
    "assets": [
      {"name": "studio-core", "version": "1.2.0", "hash": "44da..."}
    ]
  },
  "determinism": {
    "rng_seed": 1337421,
    "frame_batch": [1001, 1002],
    "config_digest": "0f97f1c...",
    "scene_digest": "b21d9a4...",
    "template": "cinematic",
    "git_revision": "cb5b7d7",
    "shader_hashes": ["aaff...", "dd12..."]
  },
  "outputs": {
    "render_path": "renders/shot010",
    "frames": [
      {"frame": 1001, "duration_ms": 412.6, "output": "renders/shot010/shot010.1001.png"},
      {"frame": 1002, "duration_ms": 410.2, "output": "renders/shot010/shot010.1002.png"}
    ],
    "warnings": []
  }
}
```

## 3. NDJSON Trace Examples
```json
{"event":"command_started","command":"render"}
{"event":"render_started","scene":"shots/shot010.json","device":"auto","frames":[1001,1002]}
{"event":"render_frame_completed","frame":1001,"duration_ms":412.6,"seed":1337421}
{"event":"render_frame_completed","frame":1002,"duration_ms":410.2,"seed":1337421}
{"event":"run_manifest_written","path":"renders/shot010/run.json","digest":"0f97f1c...","size_bytes":2048}
{"event":"render_completed","status":"success","exit_code":0,"frames":2,"warnings":0}
```

All events flow through `NdjsonEmitter`, ensuring deterministic ordering and key casing. Errors append a `command_failed` event followed by `command_completed` with non-zero exit code.

## 4. Validation Hook Checklist
- [ ] `RunManifestWriter` collects CLI args, resolved config snapshot, module/assets digests, and device info.
- [ ] Render pipeline triggers `RunManifestWriter::finalize()` after frame loop, capturing timing and output digests.
- [ ] `validate --json` emits `validation_phase_completed` events and generates run manifest preview (`run_manifest_preview`).
- [ ] CLI smoke suite asserts:
  - run manifest exists under `renders/<name>/run.json`.
  - NDJSON trace includes `run_manifest_written` event.
  - Frame timings and hash digests have deterministic formatting (fixed precision).
- [ ] Build matrix exercises CPU/GPU permutations and verifies manifest hash stability.

## 5. Pending Tasks
- Implement `RunManifestWriter` class under `cli/include/glint/cli/run_manifest_writer.h`.
- Extend `RenderCommand` scaffolding to call writer and capture frame metrics.
- Populate sample manifests per platform in `artifacts/validation/cli_build_matrix.md`.

---
Ownership: Platform Team - Milestone A (CLI Platform v1)
