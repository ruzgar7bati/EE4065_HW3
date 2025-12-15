"""
tiff_to_png.py

This script converts TIFF images to PNG format for use in markdown reports.
PNG format is better supported in markdown viewers and PDF converters.

Requires: pillow (install with: pip install pillow)

Usage: 
    python tiff_to_png.py                    # Converts all TIFF files in current directory
    python tiff_to_png.py mandrill.tiff      # Converts specific file
    python tiff_to_png.py --all              # Converts all TIFF files recursively
"""

import os
import sys
import glob
from PIL import Image

def convert_tiff_to_png(input_path, output_path=None):
    """
    Convert a TIFF image to PNG format.
    
    Args:
        input_path: Path to input TIFF file
        output_path: Path to output PNG file (optional, auto-generated if None)
    
    Returns:
        True if successful, False otherwise
    """
    if not os.path.exists(input_path):
        print(f"ERROR: Input file not found: {input_path}")
        return False
    
    # Generate output filename if not provided
    if output_path is None:
        base_name = os.path.splitext(input_path)[0]
        output_path = base_name + ".png"
    
    try:
        # Read TIFF image
        print(f"Reading TIFF image: {input_path}")
        img = Image.open(input_path)
        
        # Get image properties
        width, height = img.size
        mode = img.mode
        print(f"  Image size: {width}x{height}, Mode: {mode}")
        
        # Convert to RGB if necessary (for color images)
        if img.mode == 'RGBA':
            # Convert RGBA to RGB with white background
            rgb_img = Image.new('RGB', img.size, (255, 255, 255))
            rgb_img.paste(img, mask=img.split()[3])
            img = rgb_img
        elif img.mode not in ('RGB', 'L', 'P'):
            # Convert other modes to RGB
            img = img.convert('RGB')
        
        # Save as PNG
        img.save(output_path, 'PNG')
        
        # Verify output file was created
        if os.path.exists(output_path):
            file_size = os.path.getsize(output_path)
            print(f"  [OK] Successfully converted to: {output_path} ({file_size:,} bytes)")
            return True
        else:
            print(f"  ERROR: Output file was not created: {output_path}")
            return False
            
    except Exception as e:
        print(f"ERROR: Exception during conversion: {str(e)}")
        return False

def main():
    """Main function to handle command-line arguments and convert files."""
    
    if len(sys.argv) > 1:
        if sys.argv[1] == "--all" or sys.argv[1] == "-a":
            # Convert all TIFF files recursively
            print("Searching for all TIFF files...")
            tiff_files = glob.glob("**/*.tiff", recursive=True) + glob.glob("**/*.tif", recursive=True)
            
            if not tiff_files:
                print("No TIFF files found.")
                return
            
            print(f"Found {len(tiff_files)} TIFF file(s):")
            for tiff_file in tiff_files:
                print(f"  - {tiff_file}")
            
            print("\nConverting files...")
            success_count = 0
            for tiff_file in tiff_files:
                if convert_tiff_to_png(tiff_file):
                    success_count += 1
                print()
            
            print(f"\nConversion complete: {success_count}/{len(tiff_files)} files converted successfully.")
            
        else:
            # Convert specific file(s)
            for input_file in sys.argv[1:]:
                print(f"\n{'='*60}")
                convert_tiff_to_png(input_file)
    else:
        # Convert all TIFF files in current directory only
        print("Searching for TIFF files in current directory...")
        tiff_files = glob.glob("*.tiff") + glob.glob("*.tif")
        
        if not tiff_files:
            print("No TIFF files found in current directory.")
            print("\nUsage:")
            print("  python tiff_to_png.py                    # Convert all TIFF in current dir")
            print("  python tiff_to_png.py file.tiff          # Convert specific file")
            print("  python tiff_to_png.py --all              # Convert all TIFF recursively")
            return
        
        print(f"Found {len(tiff_files)} TIFF file(s):")
        for tiff_file in tiff_files:
            print(f"  - {tiff_file}")
        
        print("\nConverting files...")
        success_count = 0
        for tiff_file in tiff_files:
            if convert_tiff_to_png(tiff_file):
                success_count += 1
            print()
        
        print(f"\nConversion complete: {success_count}/{len(tiff_files)} files converted successfully.")

if __name__ == "__main__":
    main()
