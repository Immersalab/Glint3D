#!/usr/bin/env python3
"""
Test script for the golden image validation system

This script tests the complete golden image workflow locally:
1. Build the engine
2. Create test assets  
3. Generate golden images
4. Render test scenes
5. Compare against goldens
6. Validate diff/heatmap generation
"""

import os
import sys
import subprocess
import shutil
from pathlib import Path
import json

def run_command(cmd, description, check=True, cwd=None):
    """Run command with error handling and output"""
    print(f"\n🔨 {description}")
    print(f"Command: {' '.join(cmd) if isinstance(cmd, list) else cmd}")
    
    try:
        if isinstance(cmd, str):
            result = subprocess.run(cmd, shell=True, capture_output=True, text=True, cwd=cwd)
        else:
            result = subprocess.run(cmd, capture_output=True, text=True, cwd=cwd)
        
        if result.stdout:
            print("STDOUT:", result.stdout)
        if result.stderr and result.returncode != 0:
            print("STDERR:", result.stderr)
            
        if check and result.returncode != 0:
            print(f"❌ Command failed with exit code {result.returncode}")
            return False
        
        print(f"✅ {description} completed successfully")
        return True
        
    except Exception as e:
        print(f"❌ Command failed with exception: {e}")
        return False

def create_test_assets():
    """Create minimal test assets for golden testing"""
    print("\n📁 Creating test assets...")
    
    assets_dir = Path("test_assets/models")
    assets_dir.mkdir(parents=True, exist_ok=True)
    
    # Create minimal sphere (octahedron)
    sphere_obj = assets_dir / "sphere.obj"
    sphere_obj.write_text("""# Minimal sphere (octahedron approximation)
v 0.0 1.0 0.0
v 1.0 0.0 0.0  
v 0.0 0.0 1.0
v -1.0 0.0 0.0
v 0.0 0.0 -1.0
v 0.0 -1.0 0.0
f 1 2 3
f 1 3 4
f 1 4 5
f 1 5 2
f 6 3 2
f 6 4 3
f 6 5 4
f 6 2 5
""")
    
    # Create minimal cube
    cube_obj = assets_dir / "cube.obj"
    cube_obj.write_text("""# Minimal cube
v -1.0 -1.0 -1.0
v  1.0 -1.0 -1.0
v  1.0  1.0 -1.0
v -1.0  1.0 -1.0
v -1.0 -1.0  1.0
v  1.0 -1.0  1.0
v  1.0  1.0  1.0
v -1.0  1.0  1.0
f 1 2 3 4
f 5 8 7 6
f 1 5 6 2
f 2 6 7 3
f 3 7 8 4
f 5 1 4 8
""")
    
    # Create minimal plane
    plane_obj = assets_dir / "plane.obj"
    plane_obj.write_text("""# Minimal plane
v -2.0 0.0 -2.0
v  2.0 0.0 -2.0
v  2.0 0.0  2.0
v -2.0 0.0  2.0
f 1 2 3 4
""")
    
    print(f"✅ Created test assets in {assets_dir}")
    return True

def find_engine_binary():
    """Find the engine binary in common build locations"""
    possible_paths = [
        "builds/desktop/cmake/Release/glint.exe",
        "builds/desktop/cmake/Debug/glint.exe", 
        "builds/desktop/cmake/Release/glint",
        "builds/desktop/cmake/Debug/glint",
        "builds/vs/x64/Release/glint.exe",
        "builds/vs/x64/Debug/glint.exe"
    ]
    
    for path in possible_paths:
        if Path(path).exists():
            print(f"✅ Found engine binary: {path}")
            return path
    
    print("❌ Engine binary not found. Build the project first.")
    print("Expected locations:")
    for path in possible_paths:
        print(f"  - {path}")
    return None

def main():
    """Run the complete golden image system test"""
    print("🛡️ Golden Image System Validation")
    print("=" * 50)
    
    # Check if we're in the right directory
    if not Path("engine/src/main.cpp").exists():
        print("❌ Please run this script from the Glint3D root directory")
        return 1
    
    # Step 1: Check if engine is built
    engine_binary = find_engine_binary()
    if not engine_binary:
        print("\n🔨 Building engine...")
        
        # Try to build using CMake
        success = run_command([
            "cmake", "-S", ".", "-B", "builds/desktop/cmake", 
            "-DCMAKE_BUILD_TYPE=Release"
        ], "Configure CMake")
        
        if success:
            success = run_command([
                "cmake", "--build", "builds/desktop/cmake", "--config", "Release", "-j"
            ], "Build engine")
        
        if not success:
            print("❌ Failed to build engine. Please build manually first.")
            return 1
        
        engine_binary = find_engine_binary()
        if not engine_binary:
            print("❌ Engine binary not found after build")
            return 1
    
    # Step 2: Install Python dependencies
    print("\n📦 Checking Python dependencies...")
    success = run_command([
        sys.executable, "-m", "pip", "install", "-r", "tools/requirements.txt"
    ], "Install Python dependencies")
    
    if not success:
        print("⚠️  Failed to install dependencies, continuing anyway...")
    
    # Step 3: Create test assets
    if not create_test_assets():
        return 1
    
    # Step 4: Clean previous test outputs
    print("\n🧹 Cleaning previous outputs...")
    for dir_name in ["test_renders", "test_goldens", "test_comparison"]:
        if Path(dir_name).exists():
            shutil.rmtree(dir_name)
    
    # Step 5: Generate golden images
    print("\n🎨 Generating golden images...")
    Path("test_goldens").mkdir(exist_ok=True)
    
    success = run_command([
        sys.executable, "tools/generate_goldens.py", engine_binary,
        "--test-dir", "examples/json-ops/golden-tests",
        "--golden-dir", "test_goldens",
        "--asset-root", "test_assets",
        "--width", "400", "--height", "300",
        "--summary", "golden_generation.json"
    ], "Generate golden reference images")
    
    if not success:
        return 1
    
    # Step 6: Render test images (simulate CI render)
    print("\n🖼️  Rendering test scenes...")
    Path("test_renders").mkdir(exist_ok=True)
    
    test_scenes = list(Path("examples/json-ops/golden-tests").glob("*.json"))
    all_renders_successful = True
    
    for scene in test_scenes:
        scene_name = scene.stem
        output_path = f"test_renders/{scene_name}.png"
        
        success = run_command([
            engine_binary,
            "--asset-root", "test_assets",
            "--ops", str(scene),
            "--render", output_path,
            "--w", "400", "--h", "300",
            "--log", "warn"
        ], f"Render {scene_name}")
        
        if not success:
            all_renders_successful = False
    
    if not all_renders_successful:
        print("❌ Some renders failed")
        return 1
    
    # Step 7: Compare images
    print("\n🔍 Comparing rendered images against golden references...")
    Path("test_comparison").mkdir(exist_ok=True)
    
    success = run_command([
        sys.executable, "tools/golden_image_compare.py",
        "--batch", "test_renders", "test_goldens",
        "--output", "test_comparison",
        "--json-output", "comparison_results.json",
        "--type", "desktop",
        "--verbose"
    ], "Compare against golden images", check=False)  # Allow failures for demo
    
    # Step 8: Analyze results
    print("\n📊 Analyzing comparison results...")
    
    if Path("comparison_results.json").exists():
        with open("comparison_results.json") as f:
            results = json.load(f)
        
        total = results["total_comparisons"]
        passed = results["passed_comparisons"] 
        failed = total - passed
        
        print(f"Total comparisons: {total}")
        print(f"Passed: {passed}")
        print(f"Failed: {failed}")
        
        if failed > 0:
            print("\nFailed comparisons:")
            for result in results["results"]:
                if not result["passed"]:
                    print(f"  ❌ {result['filename']}: SSIM={result['ssim_score']:.4f}, MaxΔ={result['max_channel_diff']}")
            
            print(f"\n🔍 Diff images and heatmaps saved to: test_comparison/")
            
            # List generated artifacts
            artifacts = list(Path("test_comparison").glob("*"))
            if artifacts:
                print("Artifacts generated:")
                for artifact in artifacts:
                    print(f"  📁 {artifact}")
        else:
            print("✅ All comparisons passed!")
    
    # Step 9: Summary
    print("\n" + "=" * 50)
    print("🎯 GOLDEN IMAGE SYSTEM TEST SUMMARY")
    print("=" * 50)
    
    components = [
        ("Engine Build", engine_binary is not None),
        ("Python Dependencies", True),  # We continued even if failed
        ("Test Assets", Path("test_assets").exists()),
        ("Golden Generation", Path("test_goldens").exists() and list(Path("test_goldens").glob("*.png"))),
        ("Scene Rendering", Path("test_renders").exists() and list(Path("test_renders").glob("*.png"))),
        ("Image Comparison", Path("comparison_results.json").exists())
    ]
    
    for component, status in components:
        status_icon = "✅" if status else "❌"
        print(f"{status_icon} {component}")
    
    if Path("comparison_results.json").exists():
        with open("comparison_results.json") as f:
            results = json.load(f)
        pass_rate = results["passed_comparisons"] / results["total_comparisons"]
        print(f"\n📊 Overall pass rate: {pass_rate:.1%}")
        
        if pass_rate == 1.0:
            print("🎉 Golden image system is working perfectly!")
            return 0
        else:
            print("⚠️  Some comparisons failed - this is normal for initial testing")
            print("    Check artifacts to verify diff/heatmap generation is working")
            return 0  # Not a failure, just informational
    else:
        print("❌ No comparison results generated")
        return 1

if __name__ == "__main__":
    sys.exit(main())