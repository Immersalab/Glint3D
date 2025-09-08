Emscripten/WebGL2 Build with CMake
==================================

Requirements
- Emscripten SDK installed and activated (emsdk)
- CMake 3.15+

Build (Web)
- Windows: `emsdk activate latest && emsdk_env.bat`
- Unix/macOS: `source ./emsdk_env.sh`
- `mkdir build-web && cd build-web`
- `emcmake cmake -DCMAKE_BUILD_TYPE=Release ..`
- `cmake --build . -j`

Run (Web)
- `emrun --no_browser --port 8080 glint.html`
- Or serve the directory with any static server (ensure `.data` is served next to the `.html`)

Flags and Preloaded Assets
- Link flags: `-s USE_GLFW=3 -s FULL_ES3=1 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s MIN_WEBGL_VERSION=2 -s MAX_WEBGL_VERSION=2`
- Preloads:
  - `--preload-file Project1/assets@/assets`
  - `--preload-file Project1/shaders@/shaders`
  - `--preload-file Project1/cow-tex-fin.jpg@/assets/cow-tex-fin.jpg`

Texture Compression (KTX2)
--------------------------

- Runtime prefers `.ktx2` textures where available and falls back to PNG/JPG.
- To enable `.ktx2` loading at runtime, build with `-DENABLE_KTX2=ON` and provide `libktx` for Emscripten (or stubbed off if unavailable). When not enabled, `.ktx2` is skipped and PNG/JPG are used.
- Offline conversion: use `tools/texc.sh` or `tools/texc.ps1` to generate `.ktx2` next to existing images.
  - ETC1S (recommended default for web): `bash tools/texc.sh -r Project1/assets -m etc1s`
  - UASTC (higher quality): `bash tools/texc.sh -r Project1/assets -m uastc`
  - Windows PowerShell equivalents available.

Paths
- Shaders under `/shaders/...` (code loads `shaders/...`)
- Assets under `/assets/...`
- One-off texture `cow-tex-fin.jpg` mapped to `/assets/cow-tex-fin.jpg`

Notes
- Shader loader auto-converts `#version 330 core` → `#version 300 es` and injects precision for fragment shaders.
- WinHTTP-based AI networking is disabled on Web builds (clear error message returned).
- Compute shaders are excluded from Web builds; CPU raytracer path remains available.

Embed SDK (MVP)
----------------

A minimal embeddable wrapper is provided under `build-web/embed/`.

- `build-web/embed/embed.js` exports `createViewer(selector, { src, viewerUrl })`.
- It spawns an `<iframe>` pointing to your web build entry (`viewerUrl`, default `../index.html`) and passes initial ops via the `?state=` URL parameter.
- `viewer.applyOps(opsArray)` appends more JSON Ops v1 and reloads the iframe with updated state. A convenience mapping turns `{ op:'add_light', type:'key' }` into a neutral point light.

Example page:

```
<script type="module">
  import { createViewer } from './embed.js';
  const viewer = createViewer('#app', { src: 'assets/models/cube.obj', viewerUrl: '../index.html' });
  viewer.applyOps([{ op:'add_light', type:'key' }]);
  // You can pass any JSON Ops v1 here (see docs/json_ops_v1.md)
</script>
```

See `build-web/embed/example.html` for a complete static demo.

State Import/Export
-------------------

- Query param: `?state=` accepts a Base64URL-encoded JSON payload.
  - Encoding: RFC 4648 Base64URL (characters `A–Z a–z 0–9 - _`), padding `=` optional and ignored.
  - The payload can be either:
    - a JSON array of ops, e.g. `[ {"op":"set_camera",...}, {"op":"add_light",...} ]`, or
    - an envelope object containing `ops`, e.g. `{ "camera":{...}, "lights":[...], "ops":[...] }`.
      Only the `ops` array is applied; extra fields are metadata.
  - For convenience, if `?state=` looks like plain JSON (`{` or `[`), it is parsed directly without base64 decoding.

- Export API (web): Emscripten exports `app_share_link()` which returns a shareable URL with the current state encoded in `?state=`.
  - Usage from JS: `const getLink = Module.cwrap('app_share_link', 'string', []); const url = getLink();`
  - Exported state includes ops for camera, lights, loaded objects (with transform), and material edits where available.

- Import API (web): You can programmatically apply ops via `app_apply_ops_json(json)`.
  - Usage from JS: `const apply = Module.cwrap('app_apply_ops_json', 'number', ['string']); const ok = apply(JSON.stringify({ ops }));`
  - Returns `1` on success, `0` on error; see errors below.

Error Surfaces
--------------

- Import errors (parse, schema, unknown op, invalid parameters) are logged to the browser console via `emscripten_log`, e.g. `State import failed: ...`.
- If strict schema validation is enabled in the build/runtime, failures are reported as `Schema validation failed: ...`.
- Host apps embedding the viewer should watch the console for diagnostics during development, and may expose a wrapper that captures/relays these messages to their own UI.

Codec Notes
-----------

- The viewer now accepts both standard Base64 and Base64URL. For URLs, Base64URL is recommended to avoid `+` and `/`.
- Padding is optional on input. On export, padding is stripped for shorter share links.
