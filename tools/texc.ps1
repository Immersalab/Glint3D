# Usage:
#   powershell -ExecutionPolicy Bypass -File tools/texc.ps1 [-Root Project1/assets] [-Mode etc1s|uastc] [-Workers 4]
#
# Requires `toktx` (from KTX-Software) in PATH.
# - Mode `etc1s` produces smaller files (good default for web)
# - Mode `uastc` produces higher quality (larger); good for normal maps/UI

param(
  [string]$Root = "engine/assets",
  [ValidateSet("etc1s","uastc")] [string]$Mode = "etc1s",
  [int]$Workers = 4
)

function Has-Exe($name) {
  $p = (Get-Command $name -ErrorAction SilentlyContinue)
  return $null -ne $p
}

if (-not (Has-Exe "toktx")) {
  Write-Error "toktx not found in PATH. Install KTX-Software (toktx) and retry."
  exit 1
}

$imgExts = @('*.png','*.jpg','*.jpeg')
$files = Get-ChildItem -Path $Root -Include $imgExts -Recurse -File

if ($files.Count -eq 0) {
  Write-Host "No PNG/JPG images found under $Root"
  exit 0
}

Write-Host "Converting $($files.Count) images to KTX2 ($Mode)"

foreach ($f in $files) {
  $out = [System.IO.Path]::ChangeExtension($f.FullName, ".ktx2")
  if (Test-Path $out) {
    $fi = Get-Item $f.FullName
    $ko = Get-Item $out
    if ($ko.LastWriteTimeUtc -ge $fi.LastWriteTimeUtc) {
      Write-Host "Skip up-to-date: $($f.FullName)"
      continue
    }
  }

  $common = @(
    "--t2",
    "--genmipmap",
    "--threads=$Workers"
  )

  if ($Mode -eq "etc1s") {
    # ETC1S via BasisU (smallest). sRGB for color images.
    $args = $common + @("--bcmp", "--assign_oetf", "srgb", "-o", $out, $f.FullName)
  } else {
    # UASTC 4 effort + RDO for decent size/quality tradeoff. sRGB by default.
    $args = $common + @("--uastc", "4", "--uastc_rdo_q", "2.5", "--uastc_rdo", "--assign_oetf", "srgb", "-o", $out, $f.FullName)
  }

  Write-Host "toktx $($args -join ' ')"
  & toktx @args
  if ($LASTEXITCODE -ne 0) {
    Write-Warning "toktx failed for $($f.FullName)"
  }
}

Write-Host "Done. Generated .ktx2 next to sources."

