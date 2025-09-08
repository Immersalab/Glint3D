# UI (Web) — React + Tailwind + Vite

This is a modern web UI scaffold for the viewer. It hosts the WebGL canvas, a right Inspector, and a bottom Console, and talks to the C++ engine compiled to WASM via the existing Emscripten exports.

Quick start

1) Install prerequisites
- Node.js 18+ and pnpm or npm

2) Copy the engine build into public/engine
- Build the web target (Emscripten) from the repo root to produce `objviewer.js/.wasm/.data`.
- Copy those files into `ui/public/engine/` (same folder as this README):
  - ui/public/engine/objviewer.js
  - ui/public/engine/objviewer.wasm
  - ui/public/engine/objviewer.data (or similar name)

3) Install and run
```bash
cd ui
pnpm install   # or: npm install
pnpm dev       # or: npm run dev
```
Open the printed localhost URL. You should see a canvas, a Console, and an Inspector.

How it works
- The page includes `/engine/objviewer.js` directly. Emscripten initializes the Module and binds to the canvas the UI passes in.
- The UI calls `Module.ccall('app_apply_ops_json', 'number', ['string'], [json])` to apply JSON Ops v1. A helper SDK lives at `src/sdk/viewer.ts`.

Notes
- The current UX is minimal. It demonstrates the shape: canvas, bottom console with input, and right panel. Styling is Tailwind based.
- You can expand `src/components/Inspector.tsx` and wire more ops as needed.

