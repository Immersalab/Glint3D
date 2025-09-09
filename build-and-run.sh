#!/bin/bash
# Glint3D Build and Launch Script (Cross-platform)
# Usage: ./build-and-run.sh [debug|release] [args...]

set -e  # Exit on error

# Default to Debug if no config specified
CONFIG="Debug"
if [[ "$1" == "release" || "$1" == "Release" ]]; then
    CONFIG="Release"
    shift
elif [[ "$1" == "debug" || "$1" == "Debug" ]]; then
    CONFIG="Debug"
    shift
fi

# Collect remaining arguments
ARGS="$*"

echo
echo "========================================"
echo "  Glint3D Build and Launch Script"
echo "========================================"
echo "Configuration: $CONFIG"
echo "Arguments: $ARGS"
echo

# Step 1: Find vcpkg toolchain if available
VCPKG_TOOLCHAIN=""
if [ -f "D:/ahoqp1/Repositories/Glint/vcpkg/scripts/buildsystems/vcpkg.cmake" ]; then
    VCPKG_TOOLCHAIN="-DCMAKE_TOOLCHAIN_FILE=D:/ahoqp1/Repositories/Glint/vcpkg/scripts/buildsystems/vcpkg.cmake"
fi

# Step 1: Generate CMake project files
echo "[1/3] Generating CMake project files..."
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE="$CONFIG" $VCPKG_TOOLCHAIN

# Step 2: Build the project  
echo
echo "[2/3] Building $CONFIG configuration..."
cmake --build builds/desktop/cmake --config "$CONFIG" -j

# Step 3: Determine executable path based on platform
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "win32" || "$OSTYPE" == "cygwin" ]]; then
    # Windows (MSYS2, Git Bash, Cygwin) - Visual Studio multi-config generator
    EXECUTABLE="builds/desktop/cmake/$CONFIG/glint.exe"
else
    # Linux/macOS (single-config generators)
    EXECUTABLE="builds/desktop/cmake/glint"
fi

# Check if executable exists
if [[ ! -f "$EXECUTABLE" ]]; then
    echo "ERROR: Executable not found at $EXECUTABLE"
    echo "Available files in build directory:"
    ls -la builds/desktop/cmake/ || true
    ls -la builds/desktop/cmake/Debug/ 2>/dev/null || true
    ls -la builds/desktop/cmake/Release/ 2>/dev/null || true
    exit 1
fi

# Step 4: Launch the application
echo
echo "[3/3] Launching Glint3D..."
echo "Command: $EXECUTABLE $ARGS"
echo
echo "========================================"
echo "  Glint3D is starting..."
echo "========================================"
echo

"$EXECUTABLE" $ARGS

echo
echo "========================================"
echo "  Glint3D has exited"
echo "========================================"