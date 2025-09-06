#!/bin/bash
set -e

echo "Running Security Tests"

# Find engine binary
ENGINE_BINARY=""
if [ -f "builds/desktop/cmake/glint" ]; then
    ENGINE_BINARY="builds/desktop/cmake/glint"
elif [ -f "builds/desktop/cmake/Release/glint.exe" ]; then
    ENGINE_BINARY="builds/desktop/cmake/Release/glint.exe"
elif [ -f "builds/desktop/cmake/Debug/glint.exe" ]; then
    ENGINE_BINARY="builds/desktop/cmake/Debug/glint.exe"
else
    echo "❌ Engine binary not found. Build the project first."
    exit 1
fi

echo "Using engine binary: $ENGINE_BINARY"

# Create test results directory
mkdir -p tests/results/security

# Test 1: Path traversal attacks should be blocked
echo "Test 1: Path Traversal Protection"

# Test path traversal JSON files
if [ -d "tests/security/path_traversal" ]; then
    for attack_file in tests/security/path_traversal/*.json; do
        if [ -f "$attack_file" ]; then
            attack_name=$(basename "$attack_file" .json)
            echo "  Testing attack: $attack_name"
            
            # These should FAIL when --asset-root is set
            if timeout 10 $ENGINE_BINARY \
                --asset-root tests/data \
                --ops "$attack_file" \
                --log warn > "tests/results/logs/security_${attack_name}.log" 2>&1; then
                echo "    ❌ Security vulnerability: $attack_name was allowed!"
                exit 1
            else
                echo "    ✅ Attack blocked: $attack_name"
            fi
        fi
    done
else
    echo "  ⚠️  No path traversal tests found"
fi

# Test 2: Valid operations should work with --asset-root
echo "Test 2: Valid Operations with Asset Root"

# Create a simple valid test
cat > tests/results/security/valid_test.json << EOF
{
  "ops": [
    {
      "op": "load",
      "path": "models/sphere.obj",
      "name": "ValidSphere"
    },
    {
      "op": "set_camera",
      "position": [0, 0, 5],
      "target": [0, 0, 0]
    }
  ]
}
EOF

if timeout 30 $ENGINE_BINARY \
    --asset-root tests/data \
    --ops tests/results/security/valid_test.json \
    --render tests/results/security/valid_render.png \
    --w 128 --h 128 \
    --log warn > tests/results/logs/security_valid.log 2>&1; then
    echo "  ✅ Valid operations work with asset root"
else
    echo "  ❌ Valid operations failed with asset root"
    exit 1
fi

# Test 3: Operations without --asset-root should work (backward compatibility)
echo "Test 3: Backward Compatibility (no --asset-root)"

if timeout 30 $ENGINE_BINARY \
    --ops tests/results/security/valid_test.json \
    --render tests/results/security/backward_compat.png \
    --w 128 --h 128 \
    --log warn > tests/results/logs/security_backward_compat.log 2>&1; then
    echo "  ✅ Backward compatibility maintained"
else
    echo "  ❌ Backward compatibility broken"
    exit 1
fi

echo "✅ All security tests passed"