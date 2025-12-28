<!-- Machine Summary Block -->
{"file":"resources/templates/blank/templates/README.md","purpose":"Describes starter JSON ops templates bundled with the blank init template.","exports":["cube_scene.json","cow_showcase.json"],"depends_on":["schemas/json_ops_v1.json","resources/templates/blank/assets"],"notes":["starter_scenes","uses_workspace_relative_paths"]}
<!-- Human Summary -->
Starter JSON ops scenes for `glint init` workspaces. They reference copied template assets and use workspace-relative paths for quick rendering.

## Included Templates

- `cube_scene.json` — Simple lit cube with neutral background, renders to `renders/cube.png`.
- `cow_showcase.json` — Positions the cow model with a key light and neutral backdrop, renders to `renders/cow.png`.

All paths are workspace-relative (e.g., `assets/models/cube.obj`). Use with `glint render --ops templates/cube_scene.json`.
