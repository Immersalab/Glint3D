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
