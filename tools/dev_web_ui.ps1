param(
  [switch]$Clean
)

$ErrorActionPreference = 'Stop'

function Exec($cmd) {
  Write-Host "`n>> $cmd" -ForegroundColor Cyan
  iex $cmd
}

Push-Location (Join-Path $PSScriptRoot '..')

if ($Clean) {
  if (Test-Path build-web) { Remove-Item -Recurse -Force build-web }
}

if (!(Test-Path build-web/CMakeCache.txt)) {
  Write-Host "[1/5] Configure web build..."
  Exec 'emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release'
} else { Write-Host "build-web already configured." }

Write-Host "[2/5] Build engine (WASM)..."
Exec 'cmake --build build-web -j'

Write-Host "[3/5] Copy engine outputs to ui/public/engine..."
$eng = 'ui/public/engine'
if (!(Test-Path $eng)) { New-Item -ItemType Directory -Force -Path $eng | Out-Null }
foreach ($f in 'glint3d.js','glint3d.wasm','glint3d.data') {
  if (Test-Path ("build-web/" + $f)) {
    Copy-Item -Force ("build-web/" + $f) (Join-Path $eng $f)
    Write-Host "  copied $f"
  } else { Write-Host "  (info) build-web/$f not found, skipping." }
}

Write-Host "[4/5] Install UI deps (if needed)..."
Push-Location ui
if (!(Test-Path node_modules)) {
  Exec 'npm install'
} else { Write-Host 'node_modules already present.' }

Write-Host "[5/5] Start dev server at http://localhost:5173 ..."
Exec 'npm run dev'

Pop-Location
Pop-Location
