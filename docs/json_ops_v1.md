Schema: see `schemas/json_ops_v1.json`

JSON Ops v1.3

Overview
- Purpose: A minimal, deterministic JSON command set for scene manipulation and rendering that replays identically on desktop and web.
- Coordinate system: Right-handed, Y up, Z forward (negative Z is “into” the screen). Units are meters.
- Top-level payload: Either a single op object or an array of op objects. Execute in order.
- Naming: Use stable `name` values for objects to ensure reproducible references across platforms.

Versioning
- This document specifies version 1.3 of the JSON Ops (“JSON Ops v1.3”).
- Payloads MAY include an optional top-level `version: 1` field if sent as an object with an `ops` array, but the primary format is directly a single op object or array of op objects.

Common Types
- vec3: `[x, y, z]` three numbers.
- color3: `[r, g, b]` with 0..1 range per component.

Operations
1) load
   - Purpose: Load a model from disk and add to the scene.
   - Fields:
     - `op`: "load"
     - `path`: string (relative path)
     - `name`: string (recommended)
     - Optional: `position`: vec3, `scale`: vec3, or `transform` with `position`/`scale`
   - Example: {"op":"load","path":"assets/models/cow.obj","name":"Cow1","position":[0,0,-3]}

2) duplicate
   - Purpose: Duplicate an existing object by name.
   - Fields: `op`, `source`, `name`; optional `position` (delta), `rotation` (deg delta), `scale` (delta)
   - Example: {"op":"duplicate","source":"Cow1","name":"Cow2","position":[2,0,0]}

3) transform
   - Purpose: Apply transform to an object by name (simple setters/deltas).
   - Fields: `op`, `name`; optional `translate`: vec3, `rotate`: vec3 degrees XYZ, `scale`: vec3, `setPosition`: vec3
   - Example: {"op":"transform","name":"Cow1","translate":[1,0,0]}

4) set_material
   - Purpose: Assign material parameters inline to an object.
   - Fields: `op`, `target`, `material` with any of `color`, `roughness`, `metallic`, `specular`, `ambient`

5) add_light
   - Purpose: Add a light (`point` | `directional` | `spot`).
   - Directional: `direction`, optional `color`, `intensity`
   - Point: `position`, optional `color`, `intensity`
   - Spot: `position`, `direction`, optional `inner_deg`, `outer_deg`, `color`, `intensity`

6) set_camera
   - Purpose: Set camera by target or forward mode.
   - Fields: `position` + (`target` or `front`), optional `up`, `fov`/`fov_deg`, `near`, `far`

7) set_camera_preset
   - Purpose: Standard views relative to scene/object bounds.
   - Fields: `preset`: `front|back|left|right|top|bottom|iso_fl|iso_br` (hyphen variants also allowed: `iso-fl|iso-br`), optional `target`, `fov`, `margin`

8) orbit_camera (new)
   - Purpose: Orbit camera by yaw/pitch around a center.
   - Fields: `yaw`, `pitch`, optional `center`

9) frame_object (new)
   - Purpose: Move camera along its current view direction to frame an object by name.
   - Fields: `name`, optional `margin` (≥ 0)

10) select (new)
   - Purpose: Select an object by name for editing/gizmo.
   - Fields: `name`

11) set_background (new)
   - Purpose: Set background color or enable skybox.
   - Fields: `color` (color3) or `skybox` (string path)

12) exposure (new)
   - Purpose: Set scene exposure bias.
   - Fields: `value`: number

13) tone_map (new)
   - Purpose: Configure tone mapping operator and gamma.
   - Fields: `type`: `linear|reinhard|filmic|aces`, optional `gamma` > 0

14) render_image (renamed)
   - Purpose: Render scene to PNG (headless/CLI).
   - Fields: `path`, optional `width`, `height`

Determinism & Parity Notes
- Always provide stable `name` values for objects.
- Rotations use Euler degrees.
- Paths should be relative (e.g., `assets/models/...`, `renders/...`).
- Lights and camera apply immediately in op order.

End-to-End Example
[
  {"op":"load","path":"assets/models/cow.obj","name":"Cow1","position":[-1,0,-4]},
  {"op":"duplicate","source":"Cow1","name":"Cow2","position":[2,0,0]},
  {"op":"set_material","target":"Cow2","material":{"color":[0.8,0.2,0.2],"roughness":0.6}},
  {"op":"add_light","type":"point","position":[0,3,0],"intensity":0.8},
  {"op":"set_camera","position":[0,1,6],"target":[0,1,0],"fov_deg":45},
  {"op":"tone_map","type":"aces","gamma":2.2},
  {"op":"render_image","path":"renders/frame_0001.png","width":1280,"height":720}
]

