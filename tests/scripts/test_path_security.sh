#!/bin/bash

# Test script for path security validation
# This script tests that the --asset-root flag properly prevents path traversal

echo "🛡️  Testing Path Security Implementation"
echo "======================================="

# Create test directories
mkdir -p test_assets/models
mkdir -p test_assets/textures
echo "v 0 0 0" > test_assets/models/cube.obj
echo "test texture" > test_assets/textures/test.png

# Test 1: Valid operations with asset root should work
echo "Test 1: Valid operations with asset root..."
cat > test_valid.json << EOF
{
  "ops": [
    {
      "op": "load",
      "path": "models/cube.obj",
      "name": "ValidCube"
    }
  ]
}
EOF

if [ -f builds/desktop/cmake/Release/glint.exe ]; then
    BINARY="builds/desktop/cmake/Release/glint.exe"
elif [ -f builds/desktop/cmake/Debug/glint.exe ]; then
    BINARY="builds/desktop/cmake/Debug/glint.exe"
else
    echo "❌ No built binary found. Please build the project first."
    exit 1
fi

echo "Using binary: $BINARY"

# Run with asset root - should succeed
$BINARY --asset-root ./test_assets --ops test_valid.json --render test_valid.png --w 400 --h 300 &> /dev/null
if [ $? -eq 0 ]; then
    echo "✅ Valid operation succeeded"
else
    echo "❌ Valid operation failed (unexpected)"
fi

# Test 2: Path traversal attempts should fail
echo "Test 2: Path traversal attempts should fail..."

# Test with the negative test file
if [ -f examples/json-ops/negative-tests/path-traversal-test.json ]; then
    $BINARY --asset-root ./test_assets --ops examples/json-ops/negative-tests/path-traversal-test.json --render test_malicious.png --w 400 --h 300 &> /dev/null
    if [ $? -ne 0 ]; then
        echo "✅ Path traversal attempts properly rejected"
    else
        echo "❌ Path traversal attempts were not rejected (SECURITY ISSUE)"
    fi
else
    echo "⚠️  Negative test file not found, creating inline test..."
    
    cat > test_malicious.json << EOF
{
  "ops": [
    {
      "op": "load",
      "path": "../../../etc/passwd",
      "name": "Malicious"
    }
  ]
}
EOF
    
    $BINARY --asset-root ./test_assets --ops test_malicious.json --render test_malicious.png --w 400 --h 300 &> /dev/null
    if [ $? -ne 0 ]; then
        echo "✅ Path traversal attempts properly rejected"
    else
        echo "❌ Path traversal attempts were not rejected (SECURITY ISSUE)"
    fi
fi

# Test 3: Operations without asset root should work (backward compatibility)
echo "Test 3: Operations without asset root (backward compatibility)..."
$BINARY --ops test_valid.json --render test_no_root.png --w 400 --h 300 &> /dev/null
if [ $? -eq 0 ]; then
    echo "✅ Backward compatibility maintained"
else
    echo "❌ Backward compatibility broken"
fi

# Cleanup
echo "Cleaning up test files..."
rm -rf test_assets
rm -f test_valid.json test_malicious.json test_valid.png test_malicious.png test_no_root.png

echo "🛡️  Path security tests completed!"