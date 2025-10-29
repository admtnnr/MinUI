#!/usr/bin/env python3
"""
MinUI Visual Comparison Tool
Compares screenshots for visual regression testing
"""

import sys
import argparse
from pathlib import Path

try:
    from PIL import Image, ImageChops, ImageDraw, ImageFont
except ImportError:
    print("Error: Pillow is required")
    print("Install with: pip3 install Pillow")
    sys.exit(1)


def compare_images(image1_path, image2_path, threshold=0.95, output_path=None):
    """
    Compare two images and return similarity score

    Args:
        image1_path: Path to first image
        image2_path: Path to second image
        threshold: Similarity threshold (0-1)
        output_path: Optional path to save diff image

    Returns:
        (similarity_score, passed, diff_image)
    """
    img1 = Image.open(image1_path)
    img2 = Image.open(image2_path)

    # Ensure same size
    if img1.size != img2.size:
        print(f"Warning: Image sizes differ: {img1.size} vs {img2.size}")
        # Resize to match
        if img1.size[0] * img1.size[1] > img2.size[0] * img2.size[1]:
            img1 = img1.resize(img2.size, Image.LANCZOS)
        else:
            img2 = img2.resize(img1.size, Image.LANCZOS)

    # Convert to RGB if needed
    if img1.mode != 'RGB':
        img1 = img1.convert('RGB')
    if img2.mode != 'RGB':
        img2 = img2.convert('RGB')

    # Calculate difference
    diff = ImageChops.difference(img1, img2)

    # Calculate similarity score
    # Count pixels that are identical
    diff_data = list(diff.getdata())
    identical_pixels = sum(1 for pixel in diff_data if pixel == (0, 0, 0))
    total_pixels = len(diff_data)
    similarity = identical_pixels / total_pixels

    passed = similarity >= threshold

    # Create diff visualization
    diff_visual = diff.copy()
    # Enhance differences (make them more visible)
    diff_visual = diff_visual.point(lambda p: p * 10 if p < 255 else 255)

    # Create side-by-side comparison
    if output_path:
        width = img1.width
        height = img1.height
        comparison = Image.new('RGB', (width * 3, height + 50))

        # Add images
        comparison.paste(img1, (0, 0))
        comparison.paste(img2, (width, 0))
        comparison.paste(diff_visual, (width * 2, 0))

        # Add labels
        draw = ImageDraw.Draw(comparison)
        try:
            font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 20)
        except:
            font = ImageFont.load_default()

        draw.text((10, height + 10), "Expected", fill=(255, 255, 255), font=font)
        draw.text((width + 10, height + 10), "Actual", fill=(255, 255, 255), font=font)
        draw.text((width * 2 + 10, height + 10), "Difference (10x)", fill=(255, 255, 255), font=font)

        # Add result
        result_text = f"Similarity: {similarity*100:.2f}% - {'PASS' if passed else 'FAIL'}"
        result_color = (0, 255, 0) if passed else (255, 0, 0)
        draw.text((width, height + 30), result_text, fill=result_color, font=font)

        comparison.save(output_path)
        print(f"Comparison saved to: {output_path}")

    return similarity, passed, diff_visual


def main():
    parser = argparse.ArgumentParser(description='MinUI Visual Comparison Tool')
    parser.add_argument('expected', type=str, help='Expected/reference image')
    parser.add_argument('actual', type=str, help='Actual/test image')
    parser.add_argument('--threshold', type=float, default=0.95,
                       help='Similarity threshold (0-1, default: 0.95)')
    parser.add_argument('--output', '-o', type=str,
                       help='Output path for diff image')
    parser.add_argument('--quiet', '-q', action='store_true',
                       help='Quiet mode (only output result)')

    args = parser.parse_args()

    expected_path = Path(args.expected)
    actual_path = Path(args.actual)

    if not expected_path.exists():
        print(f"Error: Expected image not found: {expected_path}")
        return 1

    if not actual_path.exists():
        print(f"Error: Actual image not found: {actual_path}")
        return 1

    if not args.quiet:
        print(f"Comparing images:")
        print(f"  Expected: {expected_path}")
        print(f"  Actual:   {actual_path}")
        print(f"  Threshold: {args.threshold * 100}%")

    similarity, passed, diff = compare_images(
        expected_path,
        actual_path,
        args.threshold,
        args.output
    )

    if not args.quiet:
        print(f"\nSimilarity: {similarity * 100:.2f}%")
        print(f"Result: {'PASS' if passed else 'FAIL'}")

    # Exit code: 0 = pass, 1 = fail
    return 0 if passed else 1


if __name__ == '__main__':
    sys.exit(main())
