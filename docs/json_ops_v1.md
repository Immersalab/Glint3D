Schema: see  `schemas/json_ops_v1.json` 

JSON Ops v1

Overview
- Purpose: A minimal, deterministic JSON command set for scene manipulation and rendering that replays identically on desktop and web.
- Coordinate system: Right-handed, Y up, Z forward (negative Z is “into” the screen). Units are meters.
- Top-level payload: Either a single op object or an array of op objects. Execute in order.
- Naming: Use stable `name` values for objects to ensure reproducible references across platforms.

Schema: see  `schemas/json_ops_v1.json` \\n\\nVersioning
- This document specifies version 1.0 of the JSON Ops (“JSON Ops v1”).
- Payloads MAY include an optional top-level `version: "1.0"` field if sent as an object with an `ops` array, but the primary format is directly a single op object or array of op objects.

Common Types
- vec3: `[x, y, z]` three numbers.
- color3: `[r, g, b]` with 0..1 range per component.
- transform:
  - `position`: vec3
  - `rotation_deg`: vec3 Euler degrees (XYZ applied in order)
  - `scale`: vec3
  - Notes: Specify only the components you intend to change. If both `rotation_deg` and a platform-specific rotation exist, `rotation_deg` wins. No quaternions in v1.
- transform_delta: Same shape as `transform`, interpreted as deltas (e.g., `position` is an additive offset; `rotation_deg` added to current angles; `scale` multiplies component-wise).

Operations
1) load
   - Purpose: Load a model from disk and add to the scene.
   - Fields:
     - `op`: "load"
     - `path`: string (relative path). Recommended: place assets under `assets/models/`.
     - `name`: string (strongly recommended for deterministic references)
     - `transform`: transform (optional)
   - Example:
     {"op":"load","path":"assets/models/cow.obj","name":"Cow1","transform":{"position":[0,0,-3],"scale":[1,1,1]}}

2) duplicate
   - Purpose: Duplicate an existing object by name.
   - Fields:
     - `op`: "duplicate"
     - `source`: string (existing object name)
     - `name`: string (new object name)
     - `transform_delta`: transform_delta (optional)
   - Example:
     {"op":"duplicate","source":"Cow1","name":"Cow2","transform_delta":{"position":[2,0,0]}}

3) transform
   - Purpose: Transform an existing object.
   - Fields:
     - `op`: "transform"
     - `target`: string (object name)
     - `mode`: "set" | "delta" (default: "set")
     - `transform`: transform (if `mode` = "set")
     - `transform_delta`: transform_delta (if `mode` = "delta")
   - Examples:
     - Set absolute position and rotation:
       {"op":"transform","target":"Cow1","mode":"set","transform":{"position":[0,0,-5],"rotation_deg":[0,45,0]}}
     - Move right by 1 meter:
       {"op":"transform","target":"Cow1","mode":"delta","transform_delta":{"position":[1,0,0]}}

4) set_material
   - Purpose: Assign material properties to an object. If the material does not exist, it is created implicitly.
   - Fields:
     - `op`: "set_material"
     - `target`: string (object name)
     - Either:
       - `material_name`: string (assign pre-existing named material), or
       - `material`: object with any of:
         - `color`: color3 (base/diffuse)
         - `specular`: color3
         - `ambient`: color3
         - `shininess`: number
         - `roughness`: number
         - `metallic`: number
   - Examples:
     {"op":"set_material","target":"Cow1","material":{"color":[0.6,0.4,0.2],"roughness":0.8}}
     {"op":"set_material","target":"Cow2","material_name":"wood"}

5) add_light
   - Purpose: Add a light to the scene.
   - Fields:
     - `op`: "add_light"
     - `type`: "point" | "directional" | "spot"
     - If `type` = "point": `position`: vec3
     - If `type` = "directional": `direction`: vec3 (normalized or normalized during apply)
     - If `type` = "spot": `position`: vec3, `direction`: vec3, `inner_deg`: number, `outer_deg`: number
     - `color`: color3 (default [1,1,1])
     - `intensity`: number (default 1.0, non-negative)
   - Examples:
     {"op":"add_light","type":"point","position":[0,5,0],"color":[1,1,1],"intensity":0.6}
     {"op":"add_light","type":"directional","direction":[-0.5,-1,0.1]}
     {"op":"add_light","type":"spot","position":[2,3,1],"direction":[0,-1,0],"inner_deg":15,"outer_deg":25}

6) set_camera
   - Purpose: Set camera parameters.
   - Fields:
     - `op`: "set_camera"
     - Either target-mode or forward-mode:
       - Target-mode: `position`: vec3, `target`: vec3, `up`: vec3 (optional, default [0,1,0])
       - Forward-mode: `position`: vec3, `front`: vec3, `up`: vec3
     - `fov_deg`: number (default 45)
     - `near`: number (default 0.1)
     - `far`: number (default 100)
   - Example:
     {"op":"set_camera","position":[0,1,5],"target":[0,1,0],"fov_deg":45,"near":0.1,"far":100}

7) set_camera_preset
   - Purpose: Set camera to a standard view angle relative to the scene bounding sphere.
   - Fields:
     - `op`: "set_camera_preset"
     - `preset`: "front" | "back" | "left" | "right" | "top" | "bottom" | "iso_fl" | "iso_br"
     - `target`: vec3 (optional, default: scene bounding center)
     - `fov`: number (optional, default: 45 degrees)
     - `margin`: number (optional, default: 0.25 = 25% margin around bounding sphere)
   - Notes:
     - Camera position calculated from scene bounding sphere + margin
     - iso_fl = Isometric Front-Left, iso_br = Isometric Back-Right
     - Independent of prior camera state for deterministic results
   - Examples:
     {"op":"set_camera_preset","preset":"front","fov":45,"margin":0.3}
     {"op":"set_camera_preset","preset":"iso_fl","target":[0,1,0],"fov":50,"margin":0.25}

8) render
   - Purpose: Present or produce an image output.
   - Fields:
     - `op`: "render"
     - `target`: "screen" | "image" (default: "screen")
     - If `target` = "image":
       - `path`: string (desktop: filesystem path; web: used as suggested download filename)
       - `width`: integer > 0 (optional; default uses current viewport width)
       - `height`: integer > 0 (optional; default uses current viewport height)
   - Examples:
     {"op":"render","target":"screen"}
     {"op":"render","target":"image","path":"renders/frame_0001.png","width":1280,"height":720}

Determinism & Parity Notes
- Always provide stable `name` fields for objects to ensure references are unambiguous across desktop and web.
- Rotations use Euler degrees via `rotation_deg` for clarity and cross-platform parity (platforms convert internally as needed).
- Paths should be consistent and relative. Recommended: `assets/models/...` for models; `renders/...` for outputs.
- Lights and camera are applied immediately in op order; subsequent ops see the updated state.
- Web parity: `render` with `target: "image"` should trigger a download; on desktop it writes to disk. Dimensions default to the current framebuffer if not provided.

End-to-End Example
[
  {"op":"load","path":"assets/models/cow.obj","name":"Cow1","transform":{"position":[-1,0,-4]}},
  {"op":"duplicate","source":"Cow1","name":"Cow2","transform_delta":{"position":[2,0,0]}},
  {"op":"set_material","target":"Cow2","material":{"color":[0.8,0.2,0.2],"roughness":0.6}},
  {"op":"add_light","type":"point","position":[0,3,0],"intensity":0.8},
  {"op":"set_camera","position":[0,1,6],"target":[0,1,0],"fov_deg":45},
  {"op":"render","target":"screen"}
]

