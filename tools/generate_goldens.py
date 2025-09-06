#!/usr/bin/env python3
"""
Generate Golden Images for CI Testing

This script renders all golden test scenes using the Glint3D engine
and saves them as reference images for CI validation.
"""

import os
import sys
import subprocess
import json
from pathlib import Path
import argparse

class GoldenGenerator:
    """Generates golden reference images for CI testing"""
    
    def __init__(self, engine_binary: str, assets_root: str = None):
        self.engine_binary = Path(engine_binary)
        self.assets_root = assets_root
        
        if not self.engine_binary.exists():
            raise FileNotFoundError(f"Engine binary not found: {engine_binary}")
    
    def find_test_scenes(self, test_dir: str) -> list:
        """Find all JSON test scene files"""
        test_path = Path(test_dir)
        if not test_path.exists():
            raise FileNotFoundError(f"Test directory not found: {test_dir}")
        
        scenes = list(test_path.glob("*.json"))
        scenes.sort()  # Consistent ordering
        
        return scenes
    
    def render_scene(self, scene_path: Path, output_path: Path, 
                    width: int = 400, height: int = 300) -> bool:
        """Render a single scene to an output image"""
        cmd = [str(self.engine_binary)]
        
        # Add asset root if specified
        if self.assets_root:
            cmd.extend(["--asset-root", self.assets_root])
        
        # Add scene and output parameters
        cmd.extend([
            "--ops", str(scene_path),
            "--render", str(output_path),
            "--w", str(width),
            "--h", str(height),
            "--log", "warn"  # Reduce noise in output
        ])
        
        print(f"Rendering {scene_path.name} -> {output_path.name}")
        print(f"Command: {' '.join(cmd)}")
        
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=60)
            
            if result.returncode == 0:
                if output_path.exists():
                    print(f"✅ Successfully rendered {scene_path.name}")
                    return True
                else:
                    print(f"❌ Render command succeeded but output file missing: {output_path}")
                    return False
            else:
                print(f"❌ Render failed for {scene_path.name}")
                print(f"Exit code: {result.returncode}")
                if result.stdout:
                    print(f"STDOUT: {result.stdout}")
                if result.stderr:
                    print(f"STDERR: {result.stderr}")
                return False
                
        except subprocess.TimeoutExpired:
            print(f"❌ Render timeout for {scene_path.name}")
            return False
        except Exception as e:
            print(f"❌ Render error for {scene_path.name}: {e}")
            return False
    
    def generate_all_goldens(self, test_dir: str, golden_dir: str, 
                           width: int = 400, height: int = 300) -> dict:
        """Generate golden images for all test scenes"""
        golden_path = Path(golden_dir)
        golden_path.mkdir(parents=True, exist_ok=True)
        
        scenes = self.find_test_scenes(test_dir)
        results = {
            'total': len(scenes),
            'successful': 0,
            'failed': 0,
            'scenes': []
        }
        
        print(f"Found {len(scenes)} test scenes")
        print(f"Rendering at {width}x{height} to {golden_path}")
        print("-" * 60)
        
        for scene_path in scenes:
            # Create output filename (replace .json with .png)
            output_filename = scene_path.stem + ".png"
            output_path = golden_path / output_filename
            
            scene_result = {
                'scene': scene_path.name,
                'output': output_filename,
                'success': False
            }
            
            success = self.render_scene(scene_path, output_path, width, height)
            scene_result['success'] = success
            
            if success:
                results['successful'] += 1
            else:
                results['failed'] += 1
            
            results['scenes'].append(scene_result)
            print()
        
        return results

def main():
    parser = argparse.ArgumentParser(
        description="Generate golden reference images for CI testing",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Generate goldens using Release build
  python generate_goldens.py builds/desktop/cmake/Release/glint.exe
  
  # Generate with custom asset root and size  
  python generate_goldens.py builds/desktop/cmake/Debug/glint.exe \\
    --asset-root engine/assets --width 512 --height 384
        """
    )
    
    parser.add_argument('engine_binary', 
                       help='Path to Glint3D engine binary')
    parser.add_argument('--test-dir', default='examples/json-ops/golden-tests',
                       help='Directory containing test JSON files')
    parser.add_argument('--golden-dir', default='examples/golden',
                       help='Output directory for golden images')
    parser.add_argument('--asset-root', 
                       help='Asset root directory for engine')
    parser.add_argument('--width', type=int, default=400,
                       help='Render width (default: 400)')
    parser.add_argument('--height', type=int, default=300,
                       help='Render height (default: 300)')
    parser.add_argument('--summary', 
                       help='Save generation summary to JSON file')
    
    args = parser.parse_args()
    
    try:
        generator = GoldenGenerator(args.engine_binary, args.asset_root)
        
        results = generator.generate_all_goldens(
            args.test_dir, args.golden_dir, args.width, args.height
        )
        
        print("=" * 60)
        print("GENERATION SUMMARY")
        print("=" * 60)
        print(f"Total scenes: {results['total']}")
        print(f"Successful: {results['successful']}")
        print(f"Failed: {results['failed']}")
        
        if results['failed'] > 0:
            print("\nFailed scenes:")
            for scene in results['scenes']:
                if not scene['success']:
                    print(f"  - {scene['scene']}")
        
        # Save summary if requested
        if args.summary:
            with open(args.summary, 'w') as f:
                json.dump(results, f, indent=2)
            print(f"\nSummary saved to: {args.summary}")
        
        # Exit with error code if any renders failed
        return results['failed']
        
    except Exception as e:
        print(f"Error: {e}")
        return 1

if __name__ == "__main__":
    sys.exit(main())