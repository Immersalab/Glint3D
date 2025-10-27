#!/bin/bash
# Glint3D Documentation Generation Script (Cross-platform)
# Generates API documentation using Doxygen

echo "============================================"
echo "Glint3D Documentation Generator"
echo "============================================"
echo

# Check if Doxygen is installed
if ! command -v doxygen &> /dev/null; then
    echo "[ERROR] Doxygen not found in PATH"
    echo "Please install Doxygen:"
    echo "  - Ubuntu/Debian: sudo apt-get install doxygen graphviz"
    echo "  - macOS: brew install doxygen graphviz"
    echo "  - Windows: Download from https://www.doxygen.nl/download.html"
    exit 1
fi

# Change to repository root (parent directory)
cd "$(dirname "$0")/.."

# Check if Doxyfile exists
if [ ! -f "Doxyfile" ]; then
    echo "[ERROR] Doxyfile not found in repository root"
    echo "Please ensure Doxyfile exists in the repository root"
    exit 1
fi

# Generate task module documentation
echo "[INFO] Generating task module documentation..."
python3 tools/generate-task-docs.py
if [ $? -ne 0 ]; then
    echo "[WARNING] Task documentation generation failed, continuing with Doxygen..."
fi
echo

# Generate documentation
echo "[INFO] Generating API documentation..."
echo
doxygen Doxyfile

if [ $? -eq 0 ]; then
    echo
    echo "============================================"
    echo "[SUCCESS] Documentation generated successfully!"
    echo "Output: docs/api/html/index.html"
    echo "============================================"
    echo
    echo "Open docs/api/html/index.html in your browser to view the documentation"
else
    echo
    echo "[ERROR] Documentation generation failed"
    exit 1
fi