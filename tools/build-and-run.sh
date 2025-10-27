#!/bin/bash
# Glint3D Build and Run Script (Pre-RHI Release v1.0)
# Automatically checks/installs dependencies, builds, and launches

set -e

CONFIG="${1:-Debug}"
shift 2>/dev/null || true
ARGS="$@"

echo "========================================"
echo " Glint3D Build and Run Script"
echo "========================================"
echo "Configuration: $CONFIG"
echo ""

# Step 1: Check for GLFW3 dependencies
echo "[1/4] Checking dependencies..."
LIB_DIR="engine/Libraries/lib"

# Check if GLFW is available (system or vendored)
if ! pkg-config --exists glfw3 2>/dev/null && [ ! -f "$LIB_DIR/libglfw.so" ] && [ ! -f "$LIB_DIR/libglfw.dylib" ]; then
    echo "GLFW3 not found."
    echo ""
    echo "Please install GLFW3:"
    echo "  Ubuntu/Debian: sudo apt-get install libglfw3-dev"
    echo "  Fedora:        sudo dnf install glfw-devel"
    echo "  macOS:         brew install glfw"
    echo ""
    read -p "Install now using package manager? (y/N) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        if [[ "$OSTYPE" == "linux-gnu"* ]]; then
            if command -v apt-get &> /dev/null; then
                sudo apt-get update && sudo apt-get install -y libglfw3-dev
            elif command -v dnf &> /dev/null; then
                sudo dnf install -y glfw-devel
            else
                echo "ERROR: Unsupported package manager"
                exit 1
            fi
        elif [[ "$OSTYPE" == "darwin"* ]]; then
            if command -v brew &> /dev/null; then
                brew install glfw
            else
                echo "ERROR: Homebrew not found. Please install from https://brew.sh"
                exit 1
            fi
        fi
    else
        echo "ERROR: GLFW3 is required to build Glint3D"
        exit 1
    fi
else
    echo "GLFW3 found"
fi

echo ""
echo "[2/4] Configuring CMake..."
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE="$CONFIG"
if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed"
    exit 1
fi

echo ""
echo "[3/4] Building $CONFIG..."
cmake --build builds/desktop/cmake --config "$CONFIG" -j
if [ $? -ne 0 ]; then
    echo "ERROR: Build failed"
    exit 1
fi

echo ""
echo "[4/4] Launching Glint3D..."
EXE="builds/desktop/cmake/glint"
if [ ! -f "$EXE" ]; then
    echo "ERROR: Executable not found at $EXE"
    exit 1
fi

echo ""
echo "========================================"
echo " Build Complete - Launching"
echo "========================================"
echo ""

"$EXE" $ARGS
