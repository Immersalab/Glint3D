#!/bin/bash
set -e

echo "Running Integration Tests"

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

# Create test output directory
mkdir -p tests/results/integration

# Test basic JSON ops functionality
echo "Testing Basic JSON Operations..."

# Test each category of integration tests
for category in basic lighting camera materials; do
    echo "Testing $category operations..."
    
    if [ -d "tests/integration/json_ops/$category" ]; then
        for test_file in tests/integration/json_ops/$category/*.json; do
            if [ -f "$test_file" ]; then
                test_name=$(basename "$test_file" .json)
                echo "  Testing: $test_name"
                
                # Run with test assets and reasonable output settings
                if timeout 30 $ENGINE_BINARY \
                    --asset-root tests/data \
                    --ops "$test_file" \
                    --render "tests/results/integration/${category}_${test_name}.png" \
                    --w 256 --h 256 \
                    --log warn > "tests/results/logs/${category}_${test_name}.log" 2>&1; then
                    echo "    ✅ $test_name passed"
                else
                    echo "    ❌ $test_name failed"
                    echo "    Check tests/results/logs/${category}_${test_name}.log"
                    exit 1
                fi
            fi
        done
    else
        echo "  ⚠️  No tests found in $category"
    fi
done

echo "✅ All integration tests passed"