#!/usr/bin/env python3
"""
Golden Image Comparison Tool for Glint3D CI

Compares rendered images against golden reference images using SSIM and pixel-level metrics.
Generates difference images and heatmaps for failed comparisons.

Acceptance Criteria:
- Desktop: SSIM ≥ 0.995 or per-channel Δ ≤ 2 LSB
- Web vs Desktop: SSIM ≥ 0.990  
- Outputs diff + heatmap artifacts on failure
"""

import argparse
import json
import os
import sys
from pathlib import Path
from typing import Tuple, Dict, Any, Optional
import numpy as np
from PIL import Image, ImageDraw, ImageFont
import matplotlib.pyplot as plt
import matplotlib.cm as cm
from skimage.metrics import structural_similarity as ssim
from skimage.metrics import mean_squared_error
import cv2

class ImageComparisonResult:
    """Results of image comparison including metrics and validation status"""
    
    def __init__(self, 
                 ssim_score: float,
                 mse_score: float, 
                 max_channel_diff: int,
                 rmse_normalized: float,
                 passed: bool,
                 threshold_used: str,
                 diff_image_path: Optional[str] = None,
                 heatmap_path: Optional[str] = None):
        self.ssim_score = ssim_score
        self.mse_score = mse_score
        self.max_channel_diff = max_channel_diff
        self.rmse_normalized = rmse_normalized
        self.passed = passed
        self.threshold_used = threshold_used
        self.diff_image_path = diff_image_path
        self.heatmap_path = heatmap_path
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert result to dictionary for JSON serialization"""
        return {
            'ssim_score': float(self.ssim_score),
            'mse_score': float(self.mse_score),
            'max_channel_diff': int(self.max_channel_diff),
            'rmse_normalized': float(self.rmse_normalized),
            'passed': self.passed,
            'threshold_used': self.threshold_used,
            'diff_image_path': self.diff_image_path,
            'heatmap_path': self.heatmap_path
        }

class GoldenImageComparer:
    """Main comparison engine for golden image testing"""
    
    # Acceptance criteria thresholds
    DESKTOP_SSIM_THRESHOLD = 0.995
    DESKTOP_CHANNEL_DIFF_THRESHOLD = 2  # LSB (Least Significant Bit)
    WEB_VS_DESKTOP_SSIM_THRESHOLD = 0.990
    
    def __init__(self, output_dir: str = "comparison_output"):
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(exist_ok=True)
        
    def load_image(self, path: str) -> np.ndarray:
        """Load image and convert to consistent format"""
        try:
            # Load with PIL first to handle various formats
            pil_image = Image.open(path)
            
            # Convert to RGB if needed (handles RGBA, grayscale, etc.)
            if pil_image.mode != 'RGB':
                pil_image = pil_image.convert('RGB')
            
            # Convert to numpy array
            image = np.array(pil_image)
            
            return image
            
        except Exception as e:
            raise ValueError(f"Failed to load image {path}: {e}")
    
    def compute_ssim(self, img1: np.ndarray, img2: np.ndarray) -> Tuple[float, np.ndarray]:
        """Compute SSIM between two images, returning score and difference map"""
        # Ensure images are same size
        if img1.shape != img2.shape:
            raise ValueError(f"Image shape mismatch: {img1.shape} vs {img2.shape}")
        
        # Convert to grayscale for SSIM calculation
        if len(img1.shape) == 3:
            gray1 = cv2.cvtColor(img1, cv2.COLOR_RGB2GRAY)
            gray2 = cv2.cvtColor(img2, cv2.COLOR_RGB2GRAY)
        else:
            gray1, gray2 = img1, img2
        
        # Compute SSIM
        ssim_score, ssim_map = ssim(gray1, gray2, full=True, data_range=255)
        
        return ssim_score, ssim_map
    
    def compute_pixel_metrics(self, img1: np.ndarray, img2: np.ndarray) -> Tuple[float, int, float]:
        """Compute pixel-level metrics: MSE, max channel diff, normalized RMSE"""
        # Ensure same data type
        img1 = img1.astype(np.float64)
        img2 = img2.astype(np.float64)
        
        # MSE
        mse = mean_squared_error(img1, img2)
        
        # Max channel difference
        max_diff = np.max(np.abs(img1 - img2))
        
        # Normalized RMSE (0-1 scale)
        rmse_normalized = np.sqrt(mse) / 255.0
        
        return mse, int(max_diff), rmse_normalized
    
    def generate_diff_image(self, img1: np.ndarray, img2: np.ndarray, output_path: str) -> str:
        """Generate visual difference image highlighting discrepancies"""
        # Compute absolute difference
        diff = np.abs(img1.astype(np.float32) - img2.astype(np.float32))
        
        # Enhance visibility by scaling differences
        diff_enhanced = np.clip(diff * 5, 0, 255).astype(np.uint8)
        
        # Create side-by-side comparison
        h, w = img1.shape[:2]
        comparison = np.zeros((h, w * 3, 3), dtype=np.uint8)
        
        # Original, Reference, Difference
        comparison[:, :w] = img1
        comparison[:, w:2*w] = img2
        comparison[:, 2*w:] = diff_enhanced
        
        # Add labels
        pil_comparison = Image.fromarray(comparison)
        draw = ImageDraw.Draw(pil_comparison)
        
        try:
            # Try to use default font, fallback to PIL default
            font = ImageFont.load_default()
        except:
            font = None
        
        # Add text labels
        draw.text((10, 10), "Rendered", fill=(255, 255, 255), font=font)
        draw.text((w + 10, 10), "Golden", fill=(255, 255, 255), font=font)
        draw.text((2*w + 10, 10), "Difference (5x)", fill=(255, 255, 255), font=font)
        
        # Save comparison image
        pil_comparison.save(output_path)
        return output_path
    
    def generate_heatmap(self, ssim_map: np.ndarray, output_path: str) -> str:
        """Generate SSIM heatmap showing similarity across the image"""
        plt.figure(figsize=(12, 8))
        
        # Create heatmap
        plt.imshow(ssim_map, cmap='RdYlBu', vmin=0, vmax=1)
        plt.colorbar(label='SSIM Score (1.0 = identical)', shrink=0.8)
        plt.title('SSIM Similarity Heatmap\n(Blue = similar, Red = different)')
        
        # Remove axes for cleaner look
        plt.axis('off')
        
        # Save with high DPI
        plt.savefig(output_path, dpi=150, bbox_inches='tight', 
                   facecolor='white', edgecolor='none')
        plt.close()
        
        return output_path
    
    def compare_images(self, rendered_path: str, golden_path: str, 
                      comparison_type: str = "desktop") -> ImageComparisonResult:
        """
        Compare two images using acceptance criteria
        
        Args:
            rendered_path: Path to rendered/test image
            golden_path: Path to golden reference image  
            comparison_type: "desktop" or "web_vs_desktop"
        """
        # Load images
        try:
            rendered = self.load_image(rendered_path)
            golden = self.load_image(golden_path)
        except ValueError as e:
            return ImageComparisonResult(
                ssim_score=0.0, mse_score=float('inf'), max_channel_diff=255,
                rmse_normalized=1.0, passed=False, 
                threshold_used=f"ERROR: {e}"
            )
        
        # Compute metrics
        ssim_score, ssim_map = self.compute_ssim(rendered, golden)
        mse, max_channel_diff, rmse_normalized = self.compute_pixel_metrics(rendered, golden)
        
        # Determine thresholds based on comparison type
        if comparison_type == "web_vs_desktop":
            ssim_threshold = self.WEB_VS_DESKTOP_SSIM_THRESHOLD
            channel_threshold = None  # Not used for web vs desktop
            threshold_desc = f"Web vs Desktop (SSIM ≥ {ssim_threshold})"
        else:  # desktop
            ssim_threshold = self.DESKTOP_SSIM_THRESHOLD
            channel_threshold = self.DESKTOP_CHANNEL_DIFF_THRESHOLD
            threshold_desc = f"Desktop (SSIM ≥ {ssim_threshold} OR channel Δ ≤ {channel_threshold})"
        
        # Apply acceptance criteria
        if comparison_type == "web_vs_desktop":
            # Web vs Desktop: only SSIM threshold
            passed = ssim_score >= ssim_threshold
        else:
            # Desktop: SSIM OR channel difference threshold
            passed = (ssim_score >= ssim_threshold) or (max_channel_diff <= channel_threshold)
        
        # Generate artifacts if comparison failed
        diff_path = None
        heatmap_path = None
        
        if not passed:
            # Create unique filenames
            base_name = Path(rendered_path).stem
            timestamp = Path(rendered_path).stat().st_mtime
            
            diff_path = str(self.output_dir / f"{base_name}_diff_{int(timestamp)}.png")
            heatmap_path = str(self.output_dir / f"{base_name}_heatmap_{int(timestamp)}.png")
            
            # Generate visualization artifacts
            try:
                self.generate_diff_image(rendered, golden, diff_path)
                self.generate_heatmap(ssim_map, heatmap_path)
            except Exception as e:
                print(f"Warning: Failed to generate artifacts: {e}")
                diff_path = None
                heatmap_path = None
        
        return ImageComparisonResult(
            ssim_score=ssim_score,
            mse_score=mse,
            max_channel_diff=max_channel_diff,
            rmse_normalized=rmse_normalized,
            passed=passed,
            threshold_used=threshold_desc,
            diff_image_path=diff_path,
            heatmap_path=heatmap_path
        )

def main():
    """Command-line interface for golden image comparison"""
    parser = argparse.ArgumentParser(
        description="Compare rendered images against golden references",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Compare single image pair
  python golden_image_compare.py rendered.png golden.png
  
  # Compare with web vs desktop thresholds
  python golden_image_compare.py rendered.png golden.png --type web_vs_desktop
  
  # Batch compare directory
  python golden_image_compare.py --batch renders/ goldens/ --output results/
        """
    )
    
    parser.add_argument('rendered', nargs='?', help='Path to rendered image')
    parser.add_argument('golden', nargs='?', help='Path to golden reference image')
    parser.add_argument('--type', choices=['desktop', 'web_vs_desktop'], 
                       default='desktop', help='Comparison type (default: desktop)')
    parser.add_argument('--output', '-o', default='comparison_output',
                       help='Output directory for artifacts (default: comparison_output)')
    parser.add_argument('--batch', nargs=2, metavar=('RENDERED_DIR', 'GOLDEN_DIR'),
                       help='Batch compare directories')
    parser.add_argument('--json-output', help='Save results to JSON file')
    parser.add_argument('--verbose', '-v', action='store_true',
                       help='Verbose output')
    
    args = parser.parse_args()
    
    # Validate arguments
    if not args.batch and (not args.rendered or not args.golden):
        parser.error("Must provide either image pair or --batch directories")
    
    comparer = GoldenImageComparer(args.output)
    results = []
    
    if args.batch:
        # Batch comparison mode
        rendered_dir, golden_dir = args.batch
        rendered_path = Path(rendered_dir)
        golden_path = Path(golden_dir)
        
        if not rendered_path.exists():
            print(f"Error: Rendered directory {rendered_path} does not exist")
            return 1
        
        if not golden_path.exists():
            print(f"Error: Golden directory {golden_path} does not exist")
            return 1
        
        # Find matching image pairs
        rendered_images = list(rendered_path.glob("*.png"))
        total_comparisons = 0
        passed_comparisons = 0
        
        for rendered_file in rendered_images:
            golden_file = golden_path / rendered_file.name
            
            if not golden_file.exists():
                if args.verbose:
                    print(f"Warning: No golden reference for {rendered_file.name}")
                continue
            
            print(f"Comparing: {rendered_file.name}")
            
            result = comparer.compare_images(
                str(rendered_file), str(golden_file), args.type
            )
            
            result_dict = result.to_dict()
            result_dict['rendered_path'] = str(rendered_file)
            result_dict['golden_path'] = str(golden_file)
            result_dict['filename'] = rendered_file.name
            results.append(result_dict)
            
            total_comparisons += 1
            if result.passed:
                passed_comparisons += 1
                status = "✅ PASS"
            else:
                status = "❌ FAIL"
            
            print(f"  {status} - SSIM: {result.ssim_score:.6f}, "
                 f"Max Δ: {result.max_channel_diff}, Threshold: {result.threshold_used}")
            
            if args.verbose and not result.passed:
                print(f"    MSE: {result.mse_score:.2f}")
                print(f"    RMSE (norm): {result.rmse_normalized:.6f}")
                if result.diff_image_path:
                    print(f"    Diff: {result.diff_image_path}")
                if result.heatmap_path:
                    print(f"    Heatmap: {result.heatmap_path}")
        
        # Summary
        print(f"\nSummary: {passed_comparisons}/{total_comparisons} comparisons passed")
        
        if total_comparisons > 0:
            pass_rate = passed_comparisons / total_comparisons
            print(f"Pass rate: {pass_rate:.1%}")
        
    else:
        # Single comparison mode
        print(f"Comparing {args.rendered} vs {args.golden}")
        
        result = comparer.compare_images(args.rendered, args.golden, args.type)
        
        result_dict = result.to_dict()
        result_dict['rendered_path'] = args.rendered
        result_dict['golden_path'] = args.golden
        results.append(result_dict)
        
        if result.passed:
            print("✅ PASS")
        else:
            print("❌ FAIL")
        
        print(f"SSIM: {result.ssim_score:.6f}")
        print(f"Max Channel Diff: {result.max_channel_diff}")
        print(f"Threshold: {result.threshold_used}")
        
        if args.verbose:
            print(f"MSE: {result.mse_score:.2f}")
            print(f"RMSE (normalized): {result.rmse_normalized:.6f}")
        
        if not result.passed:
            if result.diff_image_path:
                print(f"Diff image: {result.diff_image_path}")
            if result.heatmap_path:
                print(f"Heatmap: {result.heatmap_path}")
    
    # Save JSON results if requested
    if args.json_output:
        summary = {
            'comparison_type': args.type,
            'total_comparisons': len(results),
            'passed_comparisons': sum(1 for r in results if r['passed']),
            'results': results
        }
        
        with open(args.json_output, 'w') as f:
            json.dump(summary, f, indent=2)
        
        print(f"Results saved to: {args.json_output}")
    
    # Exit with appropriate code
    failed_comparisons = [r for r in results if not r['passed']]
    return len(failed_comparisons)

if __name__ == "__main__":
    sys.exit(main())