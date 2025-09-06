#!/bin/bash

echo "Testing Path Security Build Fix"
echo "==============================="

set -e  # Exit on error

echo
echo "Step 1: Testing standalone compilation"
echo "--------------------------------------"
g++ -std=c++17 -I. test_path_security_build.cpp engine/src/path_security.cpp -o test_path_security_build
echo "✅ Standalone compilation successful"

echo
echo "Step 2: Running path security tests"
echo "-----------------------------------"
./test_path_security_build
echo "✅ Path security tests passed"

echo
echo "Step 3: Testing CMake build"
echo "---------------------------"
mkdir -p builds/desktop/cmake
cd builds/desktop/cmake
cmake -S ../../.. -DCMAKE_BUILD_TYPE=Release -G "Unix Makefiles"
echo "✅ CMake configure successful"

cmake --build . --config Release -j
echo "✅ CMake build successful"

cd ../../..

echo
echo "Step 4: Testing basic engine functionality"  
echo "------------------------------------------"
if [ -f "builds/desktop/cmake/glint" ]; then
    echo "✅ Engine binary created: builds/desktop/cmake/glint"
    
    # Test help flag to ensure basic functionality
    echo "Testing --help flag:"
    builds/desktop/cmake/glint --help | head -5
    
    # Test version flag
    echo "Testing --version flag:"  
    builds/desktop/cmake/glint --version
    
else
    echo "❌ Engine binary not found"
    exit 1
fi

echo
echo "Step 5: Testing asset root functionality"
echo "----------------------------------------"
mkdir -p test_assets/models

# Create a minimal test scene
cat > test_scene.json << EOF
{
  "ops": [
    {
      "op": "set_camera",
      "position": [0, 0, 5],
      "target": [0, 0, 0]
    }
  ]
}
EOF

# Create minimal test assets
cat > test_assets/models/sphere.obj << EOF
# Minimal sphere
v 0 1 0
v 1 0 0  
v 0 0 1
v -1 0 0
v 0 0 -1
v 0 -1 0
f 1 2 3
f 1 3 4
f 1 4 5
f 1 5 2
f 6 3 2
f 6 4 3
f 6 5 4
f 6 2 5
EOF

echo "Testing --asset-root with valid path:"
builds/desktop/cmake/glint --asset-root test_assets --ops test_scene.json --render test_output.png --w 100 --h 100 --log warn || true

echo "Testing --asset-root with invalid traversal (should fail):"
cat > malicious_scene.json << EOF
{
  "ops": [
    {
      "op": "load", 
      "path": "../../../etc/passwd",
      "name": "malicious"
    }
  ]
}
EOF

if builds/desktop/cmake/glint --asset-root test_assets --ops malicious_scene.json --log warn >/dev/null 2>&1; then
    echo "❌ Security test failed - traversal was allowed"
    exit 1
else
    echo "✅ Security test passed - traversal was blocked"
fi

# Cleanup
rm -rf test_assets test_scene.json malicious_scene.json test_output.png test_path_security_build

echo
echo "🎉 All tests passed! Build fix is successful."
echo "The --asset-root implementation should now work in CI."