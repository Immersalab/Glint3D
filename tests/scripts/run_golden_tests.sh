#!/bin/bash
set -e

echo "Running Golden Image Tests"

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
mkdir -p tests/results/golden/{renders,comparisons}

# Check if Python dependencies are available
if ! python3 -c "import PIL, cv2, matplotlib, numpy, skimage" 2>/dev/null; then
    echo "⚠️  Python dependencies not available, installing..."
    pip3 install -r tests/golden/tools/requirements.txt 2>/dev/null || {
        echo "❌ Failed to install Python dependencies"
        exit 1
    }
fi

# Step 1: Render test scenes
echo "Step 1: Rendering golden test scenes"
if [ -d "tests/golden/scenes" ]; then
    for scene in tests/golden/scenes/*.json; do
        if [ -f "$scene" ]; then
            scene_name=$(basename "$scene" .json)
            echo "  Rendering: $scene_name"
            
            if timeout 60 $ENGINE_BINARY \
                --asset-root tests/data \
                --ops "$scene" \
                --render "tests/results/golden/renders/${scene_name}.png" \
                --w 400 --h 300 \
                --log warn > "tests/results/logs/golden_${scene_name}.log" 2>&1; then
                echo "    ✅ Rendered $scene_name"
            else
                echo "    ❌ Failed to render $scene_name"
                exit 1
            fi
        fi
    done
else
    echo "  ⚠️  No golden test scenes found"
    exit 1
fi

# Step 2: Compare against references if they exist
echo "Step 2: Comparing against golden references"
if [ -d "tests/golden/references" ] && [ "$(ls -A tests/golden/references/*.png 2>/dev/null)" ]; then
    echo "  Found golden references, running comparison..."
    
    if python3 tests/golden/tools/golden_image_compare.py \
        --batch tests/results/golden/renders/ tests/golden/references/ \
        --output tests/results/golden/comparisons \
        --json-output tests/results/golden/comparison_results.json \
        --type desktop \
        --verbose; then
        echo "  ✅ Golden image comparison passed"
    else
        echo "  ❌ Golden image comparison failed"
        echo "  Check tests/results/golden/comparisons for diff images and heatmaps"
        exit 1
    fi
else
    echo "  ⚠️  No golden references found, generating initial set..."
    
    if python3 tests/golden/tools/generate_goldens.py "$ENGINE_BINARY" \
        --test-dir tests/golden/scenes \
        --golden-dir tests/golden/references \
        --asset-root tests/data \
        --width 400 --height 300 \
        --summary tests/results/golden/generation_summary.json; then
        echo "  ✅ Generated initial golden references"
        echo "  Commit these references to enable future regression testing"
    else
        echo "  ❌ Failed to generate golden references"
        exit 1
    fi
fi

echo "✅ Golden image tests completed"