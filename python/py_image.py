import py_serialimg
import numpy as np
import cv2 
import matplotlib.pyplot as plt
import os

# ============================================================================
# CONFIGURATION SECTION
# ============================================================================
# COM port configuration - UPDATE THIS TO MATCH YOUR STM32 BOARD
COM_PORT = "COM3"  # !!! UPDATE THIS TO YOUR NUCLEO F446RE PORT !!!

# ============================================================================
# IMAGE FILE CONFIGURATION
# ============================================================================
# All questions will be processed in sequence: Q1 → Q2 → Q3
COLOR_IMAGE_FILENAME = "mandrill.tiff"  
GRAYSCALE_IMAGE_FILENAME = "mandrill_grayscale.tiff"

# ============================================================================
# HELPER FUNCTIONS SECTION
# ============================================================================
def save_image_with_format(img, filename, format_type, question=None):
    """
    Save image with appropriate filename based on format and question.
    
    Args:
        img: Image array to save
        filename: Base filename
        format_type: Image format (GRAYSCALE, RGB565, etc.)
        question: Question number (Q1, Q2, Q3) for filename prefix
    
    Creates descriptive filenames like "Q1_received_grayscale.png"
    """
    format_name = py_serialimg.formatType[format_type].lower()
    base_name = filename.rsplit('.', 1)[0] if '.' in filename else filename
    extension = filename.rsplit('.', 1)[1] if '.' in filename else 'png'
    
    if question:
        output_filename = f"{question}_{base_name}_{format_name}.{extension}"
    else:
        output_filename = f"{base_name}_{format_name}.{extension}"
    
    cv2.imwrite(output_filename, img)
    return output_filename

def display_image_comparison(original_path, received_img, format_type, question):
    """
    Display original and received images side by side for comparison.
    
    Args:
        original_path: Path to original image file
        received_img: Image received from STM32
        format_type: Format of received image
        question: Question number for title
    """
    try:
        original = cv2.imread(original_path)
        if original is None:
            print(f"Note: Could not load original image for comparison")
            return
        
        received_height, received_width = received_img.shape[:2]
        original_resized = cv2.resize(original, (received_width, received_height))
        
        original_rgb = cv2.cvtColor(original_resized, cv2.COLOR_BGR2RGB)
        received_rgb = cv2.cvtColor(received_img, cv2.COLOR_BGR2RGB)
        
        fig, axes = plt.subplots(1, 2, figsize=(12, 6))
        
        axes[0].imshow(original_rgb)
        axes[0].set_title('Original Image', fontsize=12)
        axes[0].axis('off')
        
        format_name = py_serialimg.formatType[format_type]
        axes[1].imshow(received_rgb, cmap='gray' if format_type == py_serialimg.IMAGE_FORMAT_GRAYSCALE else None)
        axes[1].set_title(f'{question} Result from STM32 ({format_name})', fontsize=12)
        axes[1].axis('off')
        
        plt.suptitle(f'{question} - Image Processing Results', fontsize=14, fontweight='bold')
        plt.tight_layout()
        plt.show(block=False)
        plt.pause(3)
        plt.close()
        
    except Exception as e:
        print(f"Note: Could not display comparison ({e})")


# ============================================================================
# MAIN PROGRAM SECTION
# ============================================================================
print("="*70)
print("STM32 Image Processing - Homework 3")
print("="*70)
print(f"Processing Mode: Complete Cycle (Q1 → Q2 → Q3)")
print(f"COM Port: {COM_PORT}")
print("="*70)

# Check if required image files exist
if not os.path.exists(GRAYSCALE_IMAGE_FILENAME):
    if os.path.exists(COLOR_IMAGE_FILENAME):
        print(f"Note: {GRAYSCALE_IMAGE_FILENAME} not found, will use {COLOR_IMAGE_FILENAME} and convert to grayscale")
    else:
        print(f"\nERROR: Neither {GRAYSCALE_IMAGE_FILENAME} nor {COLOR_IMAGE_FILENAME} found!")
        exit(1)

if not os.path.exists(COLOR_IMAGE_FILENAME):
    print(f"\nERROR: {COLOR_IMAGE_FILENAME} not found!")
    print(f"Please ensure the file exists in the current directory.")
    exit(1)

print(f"\nImage Files:")
print(f"  Grayscale: {GRAYSCALE_IMAGE_FILENAME if os.path.exists(GRAYSCALE_IMAGE_FILENAME) else COLOR_IMAGE_FILENAME}")
print(f"  Color: {COLOR_IMAGE_FILENAME}")

# Initialize serial port
print(f"\nInitializing serial port {COM_PORT}...")
try:
    py_serialimg.SERIAL_Init(COM_PORT)
    print("✓ Serial port opened successfully")
except Exception as e:
    print(f"\nERROR: Could not open serial port: {e}")
    print(f"Please check that:")
    print(f"  1. COM port '{COM_PORT}' is correct (check Device Manager)")
    print(f"  2. No other program is using the port")
    print(f"  3. STM32 board is connected")
    exit(1)

print(f"\n{'='*70}")
print(f"Starting complete processing cycle: Q1 → Q2 → Q3")
print(f"{'='*70}")

# Variable to store Q1 binary result for Q3
q1_binary_result = None

try:
    # ========================================================================
    # Q1: Otsu's Thresholding on Grayscale Images
    # ========================================================================
    print(f"\n{'='*70}")
    print(f"Q1: Otsu's Thresholding on Grayscale Images")
    print(f"{'='*70}")
    
    # Wait for STM32 to request grayscale image
    print("\nWaiting for STM32 request (Q1)...")
    rqType, height, width, format = py_serialimg.SERIAL_IMG_PollForRequest()
    
    if rqType == py_serialimg.MCU_READS:
        # Determine which image to send (prefer grayscale, fallback to color)
        image_to_send = GRAYSCALE_IMAGE_FILENAME if os.path.exists(GRAYSCALE_IMAGE_FILENAME) else COLOR_IMAGE_FILENAME
        print(f"\n[Q1 SENDING] Sending grayscale image: {image_to_send}")
        py_serialimg.SERIAL_IMG_Write(image_to_send)
        print(f"✓ Image sent successfully")
    
    # Wait for Q1 result (binary image)
    print("\nWaiting for Q1 result...")
    rqType, height, width, format = py_serialimg.SERIAL_IMG_PollForRequest()
    
    if rqType == py_serialimg.MCU_WRITES:
        q1_binary_result = py_serialimg.SERIAL_IMG_Read()
        q1_filename = save_image_with_format(q1_binary_result, "received_from_f446re.png", format, "Q1")
        print(f"✓ Q1 result saved as: '{q1_filename}'")
        display_image_comparison(image_to_send, q1_binary_result, format, "Q1")
    
    # ========================================================================
    # Q2: Otsu's Thresholding on Color Images
    # ========================================================================
    print(f"\n{'='*70}")
    print(f"Q2: Otsu's Thresholding on Color Images")
    print(f"{'='*70}")
    
    # Wait for STM32 to request color image
    print("\nWaiting for STM32 request (Q2)...")
    rqType, height, width, format = py_serialimg.SERIAL_IMG_PollForRequest()
    
    if rqType == py_serialimg.MCU_READS:
        print(f"\n[Q2 SENDING] Sending color image: {COLOR_IMAGE_FILENAME}")
        py_serialimg.SERIAL_IMG_Write(COLOR_IMAGE_FILENAME)
        print(f"✓ Image sent successfully")
    
    # Wait for Q2 result (binary image)
    print("\nWaiting for Q2 result...")
    rqType, height, width, format = py_serialimg.SERIAL_IMG_PollForRequest()
    
    if rqType == py_serialimg.MCU_WRITES:
        q2_binary_result = py_serialimg.SERIAL_IMG_Read()
        q2_filename = save_image_with_format(q2_binary_result, "received_from_f446re.png", format, "Q2")
        print(f"✓ Q2 result saved as: '{q2_filename}'")
        display_image_comparison(COLOR_IMAGE_FILENAME, q2_binary_result, format, "Q2")
    
    # ========================================================================
    # Q3: Morphological Operations (each operation is a separate cycle)
    # ========================================================================
    print(f"\n{'='*70}")
    print(f"Q3: Morphological Operations")
    print(f"{'='*70}")
    
    # Prepare binary image file for sending (from Q1 result)
    temp_binary_file = "temp_q1_binary.png"
    cv2.imwrite(temp_binary_file, q1_binary_result)
    
    operation_names = ["erosion", "dilation", "opening", "closing"]
    
    # Each Q3 operation is a separate cycle: Send binary → Receive result
    for i, op_name in enumerate(operation_names):
        print(f"\n--- Q3 Cycle {i+1}/4: {op_name.upper()} ---")
        
        # Wait for STM32 to request binary image
        print(f"Waiting for STM32 request (Q3 {op_name})...")
        rqType, height, width, format = py_serialimg.SERIAL_IMG_PollForRequest()
        
        if rqType == py_serialimg.MCU_READS:
            print(f"\n[Q3 SENDING] Sending binary image for {op_name}")
            py_serialimg.SERIAL_IMG_Write(temp_binary_file)
            print(f"✓ Binary image sent successfully")
        
        # Wait for result
        print(f"Waiting for {op_name} result...")
        rqType, height, width, format = py_serialimg.SERIAL_IMG_PollForRequest()
        
        if rqType == py_serialimg.MCU_WRITES:
            result_img = py_serialimg.SERIAL_IMG_Read()
            q3_filename = f"Q3_{op_name}_result.png"
            cv2.imwrite(q3_filename, result_img)
            print(f"✓ Q3 {op_name} result saved as: '{q3_filename}'")
            print(f"  Cycle {i+1}/4 complete")
    
    # Clean up temp file
    os.remove(temp_binary_file)
    
    # ========================================================================
    # CYCLE COMPLETE
    # ========================================================================
    print(f"\n{'='*70}")
    print(f"✓ Complete processing cycle finished!")
    print(f"{'='*70}")
    print(f"\nResults saved:")
    print(f"  Q1: Binary thresholded image")
    print(f"  Q2: Binary thresholded image (from color)")
    print(f"  Q3: Erosion, Dilation, Opening, Closing results")
    print(f"\nAll processing complete. You can reset STM32 to run again.")

except KeyboardInterrupt:
    print("\n\n" + "="*70)
    print("Program stopped by user (Ctrl+C)")
    print("="*70)
except FileNotFoundError as e:
    print(f"\n\nERROR: File not found: {e}")
    print(f"Please ensure the image file exists in the current directory.")
except Exception as e:
    print(f"\n\nERROR: An error occurred: {e}")
    print(f"\nTroubleshooting:")
    print(f"  1. Check that COM port '{COM_PORT}' is correct")
    print(f"  2. Ensure STM32 board is connected and powered on")
    print(f"  3. Verify STM32 firmware is loaded and running")
    print(f"  4. Check that no other program is using the serial port")