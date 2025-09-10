#!/bin/bash
# Setup script to add Glint3D to PATH and create environment variables
# Run with: source setup_glint_env.sh

GLINT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GLINT_RELEASE_EXE="$GLINT_ROOT/builds/desktop/cmake/Release/glint"
GLINT_DEBUG_EXE="$GLINT_ROOT/builds/desktop/cmake/Debug/glint"

echo "Setting up Glint3D environment..."
echo
echo "Glint Root: $GLINT_ROOT"
echo "Release Exe: $GLINT_RELEASE_EXE"
echo "Debug Exe: $GLINT_DEBUG_EXE"
echo

# Check if executables exist
if [[ ! -f "$GLINT_RELEASE_EXE" ]]; then
    echo "WARNING: Release executable not found at $GLINT_RELEASE_EXE"
    echo "Please build the project first with: cmake --build builds/desktop/cmake -j"
    echo
fi

if [[ ! -f "$GLINT_DEBUG_EXE" ]]; then
    echo "WARNING: Debug executable not found at $GLINT_DEBUG_EXE"
    echo
fi

# Create glint wrapper script
cat > "$GLINT_ROOT/glint" << 'EOF'
#!/bin/bash
# Glint3D wrapper script

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GLINT_RELEASE_EXE="$SCRIPT_DIR/builds/desktop/cmake/Release/glint"
GLINT_DEBUG_EXE="$SCRIPT_DIR/builds/desktop/cmake/Debug/glint"

# Change to Glint root directory so asset paths work
cd "$SCRIPT_DIR"

# Try release build first, fallback to debug
if [[ -f "$GLINT_RELEASE_EXE" ]]; then
    "$GLINT_RELEASE_EXE" "$@"
elif [[ -f "$GLINT_DEBUG_EXE" ]]; then
    "$GLINT_DEBUG_EXE" "$@"
else
    echo "ERROR: No Glint executable found. Please build the project first."
    exit 1
fi
EOF

chmod +x "$GLINT_ROOT/glint"
echo "Created wrapper script: $GLINT_ROOT/glint"
echo

# Export environment variables for current session
export GLINT_ROOT
export PATH="$GLINT_ROOT:$PATH"

echo "Added $GLINT_ROOT to PATH for current session."
echo

# Check if we should add to shell profile
SHELL_PROFILE=""
if [[ -f "$HOME/.bashrc" ]]; then
    SHELL_PROFILE="$HOME/.bashrc"
elif [[ -f "$HOME/.zshrc" ]]; then
    SHELL_PROFILE="$HOME/.zshrc"
elif [[ -f "$HOME/.profile" ]]; then
    SHELL_PROFILE="$HOME/.profile"
fi

if [[ -n "$SHELL_PROFILE" ]]; then
    echo "To make this permanent, add the following to your $SHELL_PROFILE:"
    echo
    echo "export GLINT_ROOT=\"$GLINT_ROOT\""
    echo "export PATH=\"\$GLINT_ROOT:\$PATH\""
    echo
    
    read -p "Would you like to add this automatically? (y/N): " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        echo "" >> "$SHELL_PROFILE"
        echo "# Glint3D environment" >> "$SHELL_PROFILE"
        echo "export GLINT_ROOT=\"$GLINT_ROOT\"" >> "$SHELL_PROFILE"
        echo "export PATH=\"\$GLINT_ROOT:\$PATH\"" >> "$SHELL_PROFILE"
        echo "Added to $SHELL_PROFILE"
        echo "Restart your terminal or run: source $SHELL_PROFILE"
    else
        echo "Manual setup required - add the export commands to your shell profile."
    fi
else
    echo "Could not detect shell profile. Please manually add:"
    echo "export GLINT_ROOT=\"$GLINT_ROOT\""
    echo "export PATH=\"\$GLINT_ROOT:\$PATH\""
fi

echo
echo "Setup complete! You can now use 'glint' from any directory."
echo "Example: glint --ops examples/json-ops/sphere_basic.json --render output.png --w 512 --h 512"