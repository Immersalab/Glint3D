#!/bin/bash
# Machine Summary Block
# {"file":"tools/build-and-run.sh","purpose":"Automates dependency checks and desktop builds for Glint3D on Unix-like systems.","exports":[],"depends_on":["cmake","pkg-config"],"notes":["glfw_managed_dropin_supported","assimp_oidn_support"]}
# Human Summary
# Helper script that verifies GLFW/Doxygen, configures CMake, builds Glint3D, and optionally runs the app.
# Glint3D Build and Run Script (Pre-RHI Release v1.0)
# Automatically checks/installs dependencies, builds, and launches

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
cd "$REPO_ROOT"

CONFIG="${1:-Debug}"
shift 2>/dev/null || true

MANAGED_GLFW_DIR="third_party/managed/glfw"
MANAGED_ASSIMP_DIR="third_party/managed/assimp"
MANAGED_OIDN_DIR="third_party/managed/openimagedenoise"

has_pkg() {
    command -v pkg-config &> /dev/null && pkg-config --exists "$1"
}

has_managed_lib() {
    local dir="$1"
    local pattern="$2"
    compgen -G "${dir}/${pattern}" > /dev/null
}

resolve_glint_executable() {
    local config="${1:-$CONFIG}"
    local candidates=(
        "${REPO_ROOT}/builds/desktop/cmake/${config}/glint"
        "${REPO_ROOT}/builds/desktop/cmake/Release/glint"
        "${REPO_ROOT}/builds/desktop/cmake/RelWithDebInfo/glint"
        "${REPO_ROOT}/builds/desktop/cmake/MinSizeRel/glint"
        "${REPO_ROOT}/builds/desktop/cmake/Debug/glint"
        "${REPO_ROOT}/builds/desktop/cmake/glint"
        "${REPO_ROOT}/builds/desktop/cmake/bin/glint"
    )

    for candidate in "${candidates[@]}"; do
        if [ -x "$candidate" ]; then
            printf '%s\n' "$candidate"
            return 0
        fi
    done

    return 1
}

check_assimp() {
    if has_pkg assimp; then
        echo "Assimp found via pkg-config"
        return
    fi
    if has_managed_lib "${MANAGED_ASSIMP_DIR}/lib" "libassimp.*" || has_managed_lib "${MANAGED_ASSIMP_DIR}/lib" "assimp*.lib"; then
        echo "Assimp found in ${MANAGED_ASSIMP_DIR}"
        return
    fi

    echo "Assimp not found."
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "  Ubuntu/Debian: sudo apt-get install libassimp-dev"
        echo "  Fedora:        sudo dnf install assimp-devel"
        read -p "Install Assimp using package manager? (y/N) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            if command -v apt-get &> /dev/null; then
                sudo apt-get update && sudo apt-get install -y libassimp-dev
                return
            elif command -v dnf &> /dev/null; then
                sudo dnf install -y assimp-devel
                return
            else
                echo "ERROR: Unsupported package manager for Assimp"
                exit 1
            fi
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "  macOS: brew install assimp"
        if command -v brew &> /dev/null; then
            read -p "Install Assimp using Homebrew? (y/N) " -n 1 -r
            echo
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                brew install assimp
                return
            fi
        else
            echo "ERROR: Homebrew not found. Please install from https://brew.sh"
            exit 1
        fi
    fi

    echo "[INFO] Assimp is optional but required for extended importer support."
    echo "       Populate ${MANAGED_ASSIMP_DIR} or install via vcpkg/package manager before building."
}

check_oidn() {
    if has_pkg OpenImageDenoise; then
        echo "OpenImageDenoise found via pkg-config"
        return
    fi
    if has_managed_lib "${MANAGED_OIDN_DIR}/lib" "libOpenImageDenoise.*" || has_managed_lib "${MANAGED_OIDN_DIR}/lib" "OpenImageDenoise*.lib"; then
        echo "OpenImageDenoise found in ${MANAGED_OIDN_DIR}"
        return
    fi

    echo "OpenImageDenoise not found."
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        echo "  Ubuntu/Debian: sudo apt-get install libopenimagedenoise-dev"
        echo "  Fedora:        sudo dnf install openimagedenoise-devel"
        read -p "Install OpenImageDenoise using package manager? (y/N) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            if command -v apt-get &> /dev/null; then
                sudo apt-get update && sudo apt-get install -y libopenimagedenoise-dev
                return
            elif command -v dnf &> /dev/null; then
                sudo dnf install -y openimagedenoise-devel
                return
            else
                echo "ERROR: Unsupported package manager for OpenImageDenoise"
                exit 1
            fi
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        echo "  macOS: brew install open-image-denoise"
        if command -v brew &> /dev/null; then
            read -p "Install OpenImageDenoise using Homebrew? (y/N) " -n 1 -r
            echo
            if [[ $REPLY =~ ^[Yy]$ ]]; then
                brew install open-image-denoise
                return
            fi
        else
            echo "ERROR: Homebrew not found. Please install from https://brew.sh"
            exit 1
        fi
    fi

    echo "[INFO] OpenImageDenoise is optional but enables the denoiser pipeline."
    echo "       Populate ${MANAGED_OIDN_DIR} or install via vcpkg/package manager if required."
}

setup_glint_wrapper() {
    local bin_dir="${REPO_ROOT}/bin"
    local wrapper="${bin_dir}/glint"

    mkdir -p "$bin_dir"

    cat <<'EOF' > "$wrapper"
#!/bin/bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

candidates=(
    "$REPO_ROOT/builds/desktop/cmake/Release/glint"
    "$REPO_ROOT/builds/desktop/cmake/RelWithDebInfo/glint"
    "$REPO_ROOT/builds/desktop/cmake/MinSizeRel/glint"
    "$REPO_ROOT/builds/desktop/cmake/Debug/glint"
    "$REPO_ROOT/builds/desktop/cmake/glint"
    "$REPO_ROOT/builds/desktop/cmake/bin/glint"
)

GLINT_EXE=""
for candidate in "${candidates[@]}"; do
    if [ -x "$candidate" ]; then
        GLINT_EXE="$candidate"
        break
    fi
done

if [ -z "$GLINT_EXE" ]; then
    echo "ERROR: glint executable not found. Please run tools/build-and-run.sh first." >&2
    exit 1
fi

cd "$REPO_ROOT"
exec "$GLINT_EXE" "$@"
EOF

    chmod +x "$wrapper"

    if [[ ":$PATH:" == *":$bin_dir:"* ]]; then
        echo "[OK] ${bin_dir} is already in your PATH"
        echo "You can now run 'glint' from any directory."
    else
        cat <<EOF
[ACTION REQUIRED] Add ${bin_dir} to your PATH to use 'glint' everywhere.
  Temporary (current shell):
      export PATH="\$PATH:${bin_dir}"
  Persistent (Bash/Zsh):
      echo 'export PATH="\$PATH:${bin_dir}"' >> ~/.bashrc
      # or use ~/.zshrc for zsh
EOF
    fi
}

echo "========================================"
echo " Glint3D Build and Run Script"
echo "========================================"
echo "Configuration: $CONFIG"
echo ""

# Step 1: Check for GLFW3 dependencies
echo "[1/6] Checking dependencies..."
LIB_DIR="${MANAGED_GLFW_DIR}/lib"

# Check if GLFW is available (system or managed drop-in)
if ! pkg-config --exists glfw3 2>/dev/null && [ ! -f "$LIB_DIR/libglfw.so" ] && [ ! -f "$LIB_DIR/libglfw.dylib" ] && [ ! -f "$LIB_DIR/glfw3.lib" ]; then
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

check_assimp
check_oidn

# Check for Doxygen (optional)
if ! command -v doxygen &> /dev/null; then
    echo ""
    echo "[INFO] Doxygen not found - documentation generation will be unavailable"
    echo "Install Doxygen:"
    echo "  Ubuntu/Debian: sudo apt-get install doxygen graphviz"
    echo "  macOS: brew install doxygen graphviz"
    echo "Then run: tools/generate-docs.sh"
else
    echo "Doxygen found: ready for documentation generation"
fi

echo ""
echo "[2/6] Configuring CMake..."
cmake -S . -B builds/desktop/cmake -DCMAKE_BUILD_TYPE="$CONFIG"
if [ $? -ne 0 ]; then
    echo "ERROR: CMake configuration failed"
    exit 1
fi

echo ""
echo "[3/6] Building $CONFIG..."
cmake --build builds/desktop/cmake --config "$CONFIG" -j
if [ $? -ne 0 ]; then
    echo "ERROR: Build failed"
    exit 1
fi

echo ""
echo "[4/6] Verifying executable..."
if ! GLINT_EXE="$(resolve_glint_executable "$CONFIG")"; then
    echo "ERROR: glint executable not found. Expected it under builds/desktop/cmake (checked common Debug/Release layouts)."
    exit 1
fi
echo "Executable located at $GLINT_EXE"

echo ""
echo "[5/6] Setting up 'glint' command..."
setup_glint_wrapper

echo ""
echo "[6/6] Launching Glint3D..."
echo "========================================"
echo " Build Complete - Launching"
echo "========================================"
echo ""

"$GLINT_EXE" "$@"
