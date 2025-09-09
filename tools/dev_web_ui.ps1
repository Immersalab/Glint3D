<#
Glint3D: Refresh Web Build and Run Dev Server (PowerShell)

Usage examples:
  pwsh -File tools/dev_web_ui.ps1                # Release (default), build engine, run Vite dev
  pwsh -File tools/dev_web_ui.ps1 -Config Debug  # Debug engine
  pwsh -File tools/dev_web_ui.ps1 -SkipEngine    # Only run web UI (uses existing glint3d.*)
  pwsh -File tools/dev_web_ui.ps1 -Clean         # Clean build directory first
  pwsh -File tools/dev_web_ui.ps1 -EmsdkRoot 'D:\Graphics\emsdk'  # Load Emscripten env automatically

This script:
- Verifies toolchain (emscripten, cmake, node/npm)
- Builds WASM engine via emcmake/cmake (unless -SkipEngine)
- Copies/renames artifacts to web/public/engine (glint3d.*)
- Starts Vite dev server for the web UI
- Captures a transcript log under builds/logs/
#>

[CmdletBinding()]
param(
  [ValidateSet('Release','Debug')] [string]$Config = 'Release',
  [switch]$Clean,
  [switch]$SkipEngine,
  # Optional path to your emsdk root; if provided, the script will auto-load its env.
  [string]$EmsdkRoot = 'D:\Graphics\emsdk'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSCommandPath | Split-Path -Parent
Set-Location $root

# Logs
$logsDir = Join-Path $root 'builds/logs'
New-Item -ItemType Directory -Force -Path $logsDir | Out-Null
$ts = Get-Date -Format 'yyyyMMdd_HHmmss'
$logPath = Join-Path $logsDir "web_dev_${ts}.log"
Start-Transcript -Path $logPath -Force | Out-Null

function Write-Header($t){ Write-Host "`n=== $t ===" -ForegroundColor Cyan }
function Fail($msg){ Write-Error $msg; Stop-Transcript | Out-Null; exit 1 }

function Run-Exec {
  param(
    [Parameter(Mandatory=$true)][string]$File,
    [Parameter()][string]$Arguments = '',
    [Parameter()][string]$WorkingDir = $root,
    [Parameter()][string]$Label = ''
  )
  if ($Label) { Write-Header $Label }
  $psi = New-Object System.Diagnostics.ProcessStartInfo
  $psi.FileName = $File
  $psi.Arguments = $Arguments
  $psi.WorkingDirectory = $WorkingDir
  $psi.UseShellExecute = $false
  $psi.RedirectStandardOutput = $true
  $psi.RedirectStandardError = $true
  $p = [System.Diagnostics.Process]::Start($psi)
  $p.WaitForExit()
  $out = $p.StandardOutput.ReadToEnd()
  $err = $p.StandardError.ReadToEnd()
  if ($out) { $out | Tee-Object -FilePath $logPath -Append | Out-Host }
  if ($err) { $err | Tee-Object -FilePath $logPath -Append | Out-Host }
  if ($p.ExitCode -ne 0) {
    Fail ("Command failed ($($p.ExitCode)): `"$File`" $Arguments")
  }
}

try {
  Write-Header "Glint3D Web Refresh + Run ($Config)"

  # Tool checks
  $cmake = Get-Command cmake -ErrorAction SilentlyContinue
  if (-not $cmake) { Fail 'CMake not found on PATH' }
  $node = Get-Command node -ErrorAction SilentlyContinue
  $npm = Get-Command npm -ErrorAction SilentlyContinue
  if (-not $node -or -not $npm) { Fail 'Node.js/npm not found on PATH' }

  if (-not $SkipEngine) {
    function Import-EnvFromBatch {
      param([string]$BatchPath)
      if (-not (Test-Path $BatchPath)) { return $false }
      Write-Header "Loading Emscripten env (batch): $BatchPath"
      $psi = New-Object System.Diagnostics.ProcessStartInfo
      $psi.FileName = 'cmd.exe'
      # Important: keep the '& set' inside the cmd.exe argument string so PowerShell doesn't parse it
      $psi.Arguments = "/c call `"$BatchPath`" >nul & set"
      $psi.RedirectStandardOutput = $true
      $psi.UseShellExecute = $false
      $proc = [System.Diagnostics.Process]::Start($psi)
      $proc.WaitForExit()
      $raw = $proc.StandardOutput.ReadToEnd()
      if (-not $raw) { return $false }
      $lines = $raw -split "`r?`n" | Where-Object { $_ -and ($_ -match '=') }
      foreach ($line in $lines) {
        $idx = $line.IndexOf('=')
        if ($idx -gt 0) {
          $name = $line.Substring(0, $idx)
          $val = $line.Substring($idx+1)
          try { [Environment]::SetEnvironmentVariable($name, $val, 'Process') } catch {}
        }
      }
      return $true
    }

    function Ensure-EmsdkEnvLoaded {
      param([string]$HintPath)
      # Prefer batch env (more robust across emsdk versions)
      $bat = $null
      if ($HintPath -and (Test-Path (Join-Path $HintPath 'emsdk_env.bat'))) {
        $bat = (Join-Path $HintPath 'emsdk_env.bat')
      } elseif ($env:EMSDK -and (Test-Path (Join-Path $env:EMSDK 'emsdk_env.bat'))) {
        $bat = (Join-Path $env:EMSDK 'emsdk_env.bat')
      }
      if ($bat) {
        if (Import-EnvFromBatch -BatchPath $bat) { return }
      }
      # Fallback: dot-source PowerShell variant
      $ps1 = $null
      if ($HintPath -and (Test-Path (Join-Path $HintPath 'emsdk_env.ps1'))) {
        $ps1 = (Join-Path $HintPath 'emsdk_env.ps1')
      } elseif ($env:EMSDK -and (Test-Path (Join-Path $env:EMSDK 'emsdk_env.ps1'))) {
        $ps1 = (Join-Path $env:EMSDK 'emsdk_env.ps1')
      }
      if ($ps1) {
        Write-Header "Loading Emscripten env (ps1): $ps1"
        . $ps1
      }
    }

    $emcc = Get-Command emcc -ErrorAction SilentlyContinue
    $emcmake = Get-Command emcmake -ErrorAction SilentlyContinue
    if (-not $emcc -or -not $emcmake) {
      Ensure-EmsdkEnvLoaded -HintPath $EmsdkRoot
      $emcc = Get-Command emcc -ErrorAction SilentlyContinue
      $emcmake = Get-Command emcmake -ErrorAction SilentlyContinue
    }
    if (-not $emcc -or -not $emcmake) {
      $hint = if ($EmsdkRoot) { " (looked in '$EmsdkRoot')" } else { '' }
      $err = @(
        "",
        "Emscripten not found$hint. Please install/activate and load its environment, e.g:",
        "  git clone https://github.com/emscripten-core/emsdk.git",
        "  cd emsdk",
        "  .\\emsdk install latest",
        "  .\\emsdk activate latest",
        "  .\\emsdk_env.ps1"
      ) -join "`n"
      Fail $err
    }
  }

  # Paths
  $buildDir = Join-Path $root 'builds/web/emscripten'
  $engineOut = Join-Path $root 'web/public/engine'
  New-Item -ItemType Directory -Force -Path $buildDir,$engineOut | Out-Null

  if ($Clean) {
    Write-Header "Cleaning $buildDir"
    if (Test-Path $buildDir) { Remove-Item -Recurse -Force $buildDir }
    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
  }

  if (-not $SkipEngine) {
    # Configure with emcmake (explicit .bat to avoid PATH ambiguity)
    $emcmakeCmd = 'emcmake'
    if ($EmsdkRoot) {
      $candidate = Join-Path $EmsdkRoot 'upstream/emscripten/emcmake.bat'
      if (Test-Path $candidate) { $emcmakeCmd = $candidate }
    }
    # Ninja generator and path
    $ninjaPath = $null
    $emsdkNinja = Join-Path $EmsdkRoot 'ninja.exe'
    if (Test-Path $emsdkNinja) {
      $ninjaPath = $emsdkNinja
      $env:PATH = (Split-Path $ninjaPath) + ';' + $env:PATH
    }
    $cmakeArgs = "-S . -B `"$buildDir`" -G Ninja -DCMAKE_BUILD_TYPE=$Config"
    if ($ninjaPath) { $cmakeArgs += " -DCMAKE_MAKE_PROGRAM=`"$ninjaPath`"" }
    Run-Exec -File $emcmakeCmd -Arguments "cmake $cmakeArgs" -Label 'Configuring CMake with Emscripten'

    Run-Exec -File 'cmake' -Arguments "--build `"$buildDir`" -j" -Label 'Building WASM engine'

    Write-Header 'Deploying engine artifacts to web/public/engine'
    # Candidate base names in preference order
    $candidates = @('glint3d','objviewer','glint')
    $picked = $null
    foreach ($c in $candidates) {
      if (Test-Path (Join-Path $buildDir "$c.js")) { $picked = $c; break }
    }
    if (-not $picked) {
      Get-ChildItem $buildDir | Write-Host
      Fail "No engine artifacts found in $buildDir"
    }

    # Copy + rename to glint3d.*
    Copy-Item (Join-Path $buildDir "$picked.js")   (Join-Path $engineOut 'glint3d.js')   -Force
    Copy-Item (Join-Path $buildDir "$picked.wasm") (Join-Path $engineOut 'glint3d.wasm') -Force
    if (Test-Path (Join-Path $buildDir "$picked.data")) {
      Copy-Item (Join-Path $buildDir "$picked.data") (Join-Path $engineOut 'glint3d.data') -Force
    }
    if (Test-Path (Join-Path $buildDir "$picked.html")) {
      Copy-Item (Join-Path $buildDir "$picked.html") (Join-Path $engineOut 'glint3d.html') -Force
    }

    # Warn if the JS glue still mentions objviewer; frontend maps it via locateFile
    $jsText = Get-Content -Raw (Join-Path $engineOut 'glint3d.js')
    if ($jsText -match 'objviewer\.data|objviewer\.wasm') {
      Write-Warning 'Engine JS references objviewer.*; frontend locateFile remaps to glint3d.*'
    }
  }

  # Verify deployed engine files
  Write-Header 'Verifying deployed engine files'
  $expect = @('glint3d.js','glint3d.wasm','glint3d.data') | ForEach-Object { Join-Path $engineOut $_ }
  foreach ($f in $expect) {
    if (-not (Test-Path $f)) { Write-Warning "Missing: $f" } else { Get-Item $f | Select-Object Name,Length,LastWriteTime | Format-Table | Out-String | Write-Host }
  }

  # Start web dev server (Vite)
  Write-Header 'Starting Vite dev server (web)'
  $env:NODE_ENV = 'development'
  # Use workspace to ensure correct package
  & npm run dev --workspace=web 2>&1 | Tee-Object -FilePath $logPath -Append

} catch {
  Write-Error "Unhandled error: $($_.Exception.Message)"
  Write-Error $_.ScriptStackTrace
  exit 1
} finally {
  try { Stop-Transcript | Out-Null } catch {}
  Write-Host "`nLog saved to: $logPath"
}
