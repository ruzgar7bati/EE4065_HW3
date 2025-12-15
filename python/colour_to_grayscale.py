"""
convert_to_grayscale.py

This script converts a color image (mandrill.tiff) to grayscale format.
The grayscale version can then be used for sending to STM32 for Otsu thresholding.

Usage: python convert_to_grayscale.py
"""

import cv2
import numpy as np
import os

# ============================================================================
# CONFIGURATION SECTION
# ============================================================================
# Input file: The original color image we want to convert
INPUT_IMAGE = "python/mandrill.tiff"

# Output file: Where to save the grayscale version
OUTPUT_IMAGE = "python/mandrill_grayscale.tiff"

# Target size for STM32 (optional - you can resize here or let STM32 handle it)
# Set to None if you want to keep original size, or (width, height) tuple
TARGET_SIZE = (128, 128)  # STM32 uses 128x128, but you can keep original size too

# ============================================================================
# IMAGE LOADING SECTION
# ============================================================================
# Check if input file exists before proceeding
if not os.path.exists(INPUT_IMAGE):
    print(f"ERROR: Input image '{INPUT_IMAGE}' not found!")
    print(f"Please make sure the file exists in the current directory.")
    exit(1)

# Load the color image using OpenCV
# cv2.imread() loads images in BGR format by default
print(f"Loading image: {INPUT_IMAGE}")
original_image = cv2.imread(INPUT_IMAGE)

# Check if image was loaded successfully
if original_image is None:
    print(f"ERROR: Could not load image '{INPUT_IMAGE}'")
    print(f"Please check if the file is a valid image format.")
    exit(1)

# Display original image properties
original_height, original_width = original_image.shape[:2]
print(f"Original image size: {original_width}x{original_height}")
print(f"Original image channels: {original_image.shape[2] if len(original_image.shape) == 3 else 1}")

# ============================================================================
# COLOR TO GRAYSCALE CONVERSION SECTION
# ============================================================================
# Convert BGR (Blue-Green-Red) color image to grayscale
# OpenCV uses the formula: Gray = 0.299*R + 0.587*G + 0.114*B
# This is the standard luminance formula that matches human eye sensitivity
print(f"\nConverting to grayscale...")
grayscale_image = cv2.cvtColor(original_image, cv2.COLOR_BGR2GRAY)

# Display grayscale image properties
gray_height, gray_width = grayscale_image.shape
print(f"Grayscale image size: {gray_width}x{gray_height}")
print(f"Grayscale image is single channel (2D array)")

# ============================================================================
# RESIZING SECTION (Optional)
# ============================================================================
# Resize the grayscale image to target size if specified
# This is useful if you want to pre-process the image to STM32's expected size
if TARGET_SIZE is not None:
    print(f"\nResizing to {TARGET_SIZE[0]}x{TARGET_SIZE[1]}...")
    # cv2.resize() uses (width, height) format, not (height, width)
    resized_image = cv2.resize(grayscale_image, TARGET_SIZE, interpolation=cv2.INTER_AREA)
    final_image = resized_image
    print(f"Resized image size: {TARGET_SIZE[0]}x{TARGET_SIZE[1]}")
else:
    # Keep original size
    final_image = grayscale_image
    print(f"\nKeeping original size: {gray_width}x{gray_height}")

# ============================================================================
# IMAGE SAVING SECTION
# ============================================================================
# Save the grayscale image to disk
# OpenCV can save in various formats: .png, .jpg, .tiff, etc.
print(f"\nSaving grayscale image to: {OUTPUT_IMAGE}")
success = cv2.imwrite(OUTPUT_IMAGE, final_image)

# Check if save was successful
if success:
    print(f"✓ Successfully saved grayscale image!")
    print(f"  File: {OUTPUT_IMAGE}")
    print(f"  Size: {final_image.shape[1]}x{final_image.shape[0]} pixels")
    print(f"  Format: Grayscale (8-bit, single channel)")
else:
    print(f"ERROR: Failed to save image to '{OUTPUT_IMAGE}'")
    exit(1)

# ============================================================================
# VERIFICATION SECTION
# ============================================================================
# Optional: Display both images side by side for verification
# This helps you visually confirm the conversion worked correctly
print(f"\nDisplaying images for verification (press any key to close)...")
try:
    # Create a side-by-side comparison
    # If we resized, show original grayscale and resized version
    if TARGET_SIZE is not None:
        # Stack images horizontally for comparison
        comparison = np.hstack((grayscale_image, resized_image))
        cv2.imshow("Original Grayscale (left) | Resized Grayscale (right)", comparison)
    else:
        # Just show the grayscale image
        cv2.imshow("Grayscale Image", final_image)
    
    # Wait for user to press any key (0 means wait indefinitely)
    cv2.waitKey(0)
    cv2.destroyAllWindows()
    print("Display closed.")
except Exception as e:
    print(f"Note: Could not display images ({e}), but file was saved successfully.")

# ============================================================================
# COMPLETION MESSAGE
# ============================================================================
print(f"\n{'='*60}")
print(f"Conversion complete!")
print(f"{'='*60}")
print(f"Input:  {INPUT_IMAGE} (color)")
print(f"Output: {OUTPUT_IMAGE} (grayscale)")
print(f"\nYou can now use '{OUTPUT_IMAGE}' for sending to STM32.")
print(f"{'='*60}")