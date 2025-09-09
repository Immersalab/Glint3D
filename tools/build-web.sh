#!/bin/bash

# Glint3D Web Build Script
# Builds both the engine (WASM) and web frontend

set -e  # Exit on any error

echo "=== Glint3D Web Build Script ==="

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_step() {
    echo -e "${BLUE}>>> $1${NC}"
}

print_success() {
    echo -e "${GREEN}✓ $1${NC}"
}

print_warning() {
    echo -e "${YELLOW}⚠ $1${NC}"
}

print_error() {
    echo -e "${RED}✗ $1${NC}"
}

# Check if we're in the right directory
if [[ ! -f "CMakeLists.txt" ]] || [[ ! -d "engine" ]]; then
    print_error "Must be run from the glint3d root directory"
    exit 1
fi

# Default build type
BUILD_TYPE=${1:-Release}

print_step "Building Glint3D Web (${BUILD_TYPE})"

# Step 1: Build engine for web (WASM)
print_step "Step 1: Building C++ engine for WASM"

# Check if emscripten is available
if ! command -v emcc &> /dev/null; then
    print_error "Emscripten not found. Please install and activate the Emscripten SDK:"
    echo "  git clone https://github.com/emscripten-core/emsdk.git"
    echo "  cd emsdk"
    echo "  ./emsdk install latest"
    echo "  ./emsdk activate latest"
    echo "  source ./emsdk_env.sh"
    exit 1
fi

# Create build directory
mkdir -p builds/web/emscripten

# Configure with emscripten
print_step "Configuring CMake with Emscripten..."
emcmake cmake -S . -B builds/web/emscripten -DCMAKE_BUILD_TYPE=${BUILD_TYPE}

# Build the engine
print_step "Compiling C++ engine to WASM..."
cmake --build builds/web/emscripten -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

print_success "Engine WASM build completed"

# Step 2: Copy engine artifacts to web frontend
print_step "Step 2: Copying WASM artifacts to web frontend"

# Create engine directory in web public folder
mkdir -p web/public/engine

# Copy engine files (handle both possible output names and rename as needed)
if [[ -f "builds/web/emscripten/glint3d.js" ]]; then
    cp builds/web/emscripten/glint3d.* web/public/engine/
    print_success "Copied glint3d.* files to web/public/engine/"
elif [[ -f "builds/web/emscripten/objviewer.js" ]]; then
    # Copy and rename objviewer -> glint3d for frontend compatibility
    cp builds/web/emscripten/objviewer.js web/public/engine/glint3d.js
    cp builds/web/emscripten/objviewer.wasm web/public/engine/glint3d.wasm
    cp builds/web/emscripten/objviewer.data web/public/engine/glint3d.data
    [[ -f "builds/web/emscripten/objviewer.html" ]] && cp builds/web/emscripten/objviewer.html web/public/engine/glint3d.html
    print_success "Copied and renamed objviewer.* -> glint3d.* files to web/public/engine/"
elif [[ -f "builds/web/emscripten/glint.js" ]]; then
    # Handle glint target name
    cp builds/web/emscripten/glint.js web/public/engine/glint3d.js
    cp builds/web/emscripten/glint.wasm web/public/engine/glint3d.wasm
    cp builds/web/emscripten/glint.data web/public/engine/glint3d.data
    [[ -f "builds/web/emscripten/glint.html" ]] && cp builds/web/emscripten/glint.html web/public/engine/glint3d.html
    print_success "Copied and renamed glint.* -> glint3d.* files to web/public/engine/"
else
    print_error "No engine WASM artifacts found. Expected glint3d.*, objviewer.*, or glint.* files."
    exit 1
fi

# Step 3: Build packages if needed
print_step "Step 3: Building TypeScript packages"
npm run build:packages

print_success "Packages built successfully"

# Step 4: Build web frontend
print_step "Step 4: Building React web frontend"
npm run build:web

print_success "Web frontend built successfully"

# Step 5: Show build summary
print_step "Build Summary"

echo "Engine WASM artifacts:"
ls -la builds/web/emscripten/*.{js,wasm,data,html} 2>/dev/null || true

echo
echo "Web frontend build:"
ls -la web/dist/

echo
echo "Web engine assets:"
ls -la web/public/engine/

print_success "Web build completed successfully!"

echo
print_step "To serve the web build:"
echo "  npm run dev             # Development server"
echo "  npm run preview         # Preview production build"
echo "  emrun --port 8080 builds/web/emscripten/glint3d.html  # Serve WASM directly"

exit 0