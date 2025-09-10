#!/usr/bin/env bash
set -euo pipefail

# --- Locate project + roots ---
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"
CMAKELISTS="$PROJECT_ROOT/CMakeLists.txt"
if [ ! -f "$CMAKELISTS" ]; then
  echo "Error: Run this from the Glint3D folder (where CMakeLists.txt lives)."
  exit 1
fi
GLINT_ROOT="$(cd "$PROJECT_ROOT/.." && pwd)"

# --- Find or install vcpkg (supports 'vcpkg' or 'vckpg') ---
VCPKG_DIR=""
if [ -d "$GLINT_ROOT/vcpkg" ]; then
  VCPKG_DIR="$GLINT_ROOT/vcpkg"
elif [ -d "$GLINT_ROOT/vckpg" ]; then
  VCPKG_DIR="$GLINT_ROOT/vckpg"
else
  echo "vcpkg not found next to Glint3D. Cloning into '$GLINT_ROOT/vcpkg'..."
  git clone https://github.com/microsoft/vcpkg "$GLINT_ROOT/vcpkg"
  VCPKG_DIR="$GLINT_ROOT/vcpkg"
fi

# --- Bootstrap vcpkg if needed ---
if [ ! -f "$VCPKG_DIR/vcpkg.exe" ]; then
  echo "Bootstrapping vcpkg..."
  ( cd "$VCPKG_DIR" && cmd //c bootstrap-vcpkg.bat )
fi

# --- Install dependencies ---
echo "Installing glfw3 (x64-windows) via vcpkg..."
"$VCPKG_DIR/vcpkg.exe" install glfw3:x64-windows

# (optional) integrate with MSVC; safe to re-run
"$VCPKG_DIR/vcpkg.exe" integrate install || true

# --- Toolchain path (Windows-style for MSVC CMake) ---
if command -v cygpath >/dev/null 2>&1; then
  TOOLCHAIN="$(cygpath -w "$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake")"
else
  TOOLCHAIN="$VCPKG_DIR/scripts/buildsystems/vcpkg.cmake"
fi

# --- Wipe stale CMake cache if source dir changed ---
BUILD_ROOT="$PROJECT_ROOT/builds/desktop/cmake"
BUILD_DIR="$BUILD_ROOT/Debug"
CACHE="$BUILD_DIR/CMakeCache.txt"

if [ -f "$CACHE" ]; then
  SRC_IN_CACHE="$(grep -E '^CMAKE_HOME_DIRECTORY:INTERNAL=' "$CACHE" | sed 's/.*=//')"
  # Current dir in Windows style if available (helps comparison)
  if command -v pwd -W >/dev/null 2>&1; then
    CUR_SRC="$(pwd -W)"
  else
    CUR_SRC="$PROJECT_ROOT"
  fi
  if [ "$SRC_IN_CACHE" != "$CUR_SRC" ] && [ "$SRC_IN_CACHE" != "$PROJECT_ROOT" ]; then
    echo "Detected source dir change. Removing '$BUILD_ROOT'..."
    rm -rf "$BUILD_ROOT"
  fi
fi

# --- Configure with MSVC generator ---
echo "Configuring with CMake..."
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" -G "Visual Studio 17 2022" \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN"

# --- Build ---
echo "Building (Debug)..."
cmake --build "$BUILD_DIR" --config Debug

echo
echo "Done."
echo "Solution: $BUILD_DIR/Glint3D.sln"
echo "If you want to run the exe, check one of:"
echo "  $BUILD_DIR/Debug/glint.exe"
echo "  $BUILD_DIR/glint.exe"
