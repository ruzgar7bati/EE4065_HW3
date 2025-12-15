import numpy as np
import serial
import msvcrt
import cv2
import time

# ============================================================================
# CONSTANTS SECTION
# ============================================================================
# Request type constants: MCU_WRITES means STM32 sends image to PC,
# MCU_READS means STM32 requests image from PC
MCU_WRITES = 87
MCU_READS  = 82

# Request type dictionary for readable output
rqType = { MCU_WRITES: "MCU Sends Image", MCU_READS: "PC Sends Image"} 

# Image format constants matching STM32 definitions
IMAGE_FORMAT_GRAYSCALE	= 1  # 1 byte per pixel (0-255)
IMAGE_FORMAT_RGB565		= 2  # 2 bytes per pixel (5-6-5 bit color)
IMAGE_FORMAT_RGB888		= 3  # 3 bytes per pixel (full color)

# Format dictionary for readable output
formatType = { 
    IMAGE_FORMAT_GRAYSCALE: "Grayscale", 
    IMAGE_FORMAT_RGB565: "RGB565", 
    IMAGE_FORMAT_RGB888: "RGB888"
} 

# ============================================================================
# SERIAL PORT INITIALIZATION SECTION
# ============================================================================
def SERIAL_Init(port):
    """
    Initialize serial communication with STM32.
    
    Args:
        port: COM port string (e.g., "COM6")
    
    Opens serial port at 2 Mbps baud rate for high-speed image transfer.
    """
    global __serial    
    __serial = serial.Serial(port, 2000000, timeout = 10)
    __serial.flush()  # Clear any existing data in buffers
    print(__serial.name, "Opened")
    print("")

# ============================================================================
# REQUEST POLLING SECTION
# ============================================================================
def SERIAL_IMG_PollForRequest():
    """
    Wait for and parse image transfer request from STM32.
    
    Protocol format: "ST" (2 bytes) + RequestType (1 byte) + Height (2 bytes) + 
                     Width (2 bytes) + Format (1 byte)
    
    Returns:
        [requestType, height, width, format] as list of integers
    
    The function continuously polls the serial port until it detects the "ST" 
    header sequence, then reads the request parameters.
    """
    global requestType
    global height
    global width
    global format
    global imgSize
    
    while(1):
        # Allow user to exit with ESC key
        if msvcrt.kbhit() and msvcrt.getch() == chr(27).encode():
            print("Exit program!")
            exit(0)
        
        # Read one byte and check if we got data
        byte1 = __serial.read(1)
        if len(byte1) == 0:
            continue  # No data available, keep waiting
        
        # Convert to numpy array and extract scalar value
        byte1_val = np.frombuffer(byte1, dtype=np.uint8)[0]
        
        # Look for "ST" header (ASCII: S=83, T=84)
        if byte1_val == 83:
            # Read second byte
            byte2 = __serial.read(1)
            if len(byte2) == 0:
                continue  # Incomplete header, keep waiting
            
            byte2_val = np.frombuffer(byte2, dtype=np.uint8)[0]
            
            if byte2_val == 84:
                # Parse request parameters
                requestType_bytes = __serial.read(1)
                height_bytes = __serial.read(2)
                width_bytes = __serial.read(2)
                format_bytes = __serial.read(1)
                
                # Check if we got all the data
                if (len(requestType_bytes) == 1 and len(height_bytes) == 2 and 
                    len(width_bytes) == 2 and len(format_bytes) == 1):
                    
                    requestType  = int(np.frombuffer(requestType_bytes, dtype=np.uint8)[0])
                    height       = int(np.frombuffer(height_bytes, dtype=np.uint16)[0])
                    width        = int(np.frombuffer(width_bytes, dtype=np.uint16)[0])
                    format       = int(np.frombuffer(format_bytes, dtype=np.uint8)[0])
                    imgSize     = height * width * format
                    
                    # Display received request information
                    print("Request Type : ", rqType[int(requestType)])
                    print("Height       : ", int(height))
                    print("Width        : ", int(width))
                    print("Format       : ", formatType[int(format)])
                    print()
                    return [int(requestType), int(height), int(width), int(format)]

# ============================================================================
# IMAGE RECEIVING SECTION (MCU -> PC)
# ============================================================================
def SERIAL_IMG_Read():
    """
    Receive image data from STM32 and convert to displayable format.
    
    Reads imgSize bytes from serial port, reshapes into image array,
    and converts format for OpenCV display.
    
    Returns:
        numpy array: Image in BGR format (3 channels) for display/saving
    
    Note: Grayscale images are converted to BGR (3-channel) for compatibility
    with OpenCV display functions, but the actual data is grayscale.
    """
    # Read raw image bytes from serial port
    img = np.frombuffer(__serial.read(imgSize), dtype = np.uint8)
    
    # Reshape based on format
    # For grayscale: (height, width) - 2D array
    # For color: (height, width, channels) - 3D array
    if format == IMAGE_FORMAT_GRAYSCALE:
        img = np.reshape(img, (height, width))
    else:
        img = np.reshape(img, (height, width, format))
    
    # Convert to BGR format for OpenCV display/saving
    # Grayscale images need to be converted to 3-channel BGR for cv2.imshow/imwrite
    if format == IMAGE_FORMAT_GRAYSCALE:
        # Convert single-channel grayscale to 3-channel BGR (for display)
        # This preserves the grayscale appearance but makes it compatible with OpenCV
        img = cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)
    elif format == IMAGE_FORMAT_RGB565:
        # Convert RGB565 (16-bit color) to BGR (24-bit color)
        img = cv2.cvtColor(img, cv2.COLOR_BGR5652BGR)
    # RGB888 doesn't need conversion, but OpenCV expects BGR, so we might need to swap
    # For now, assuming STM32 sends in BGR format if RGB888

    # Display image for 2 seconds
    timestamp = time.strftime('%Y_%m_%d_%H%M%S', time.localtime())     
    cv2.imshow("Received Image", img) 
    cv2.waitKey(2000)
    cv2.destroyAllWindows()
    
    return img

# ============================================================================
# IMAGE SENDING SECTION (PC -> MCU)
# ============================================================================
def SERIAL_IMG_Write(path):
    """
    Load image from file, convert to requested format, and send to STM32.
    
    Args:
        path: File path to image (can be color or grayscale)
    
    The function automatically:
    1. Loads the image (color or grayscale)
    2. Resizes to STM32's requested dimensions
    3. Converts to requested format (GRAYSCALE or RGB565)
    4. Sends raw bytes over serial
    
    Supports both color and grayscale input files. If input is grayscale and
    STM32 requests RGB565, it will convert grayscale to color.
    """
    # Load image from file
    # cv2.imread() returns:
    # - Color image: (height, width, 3) BGR format
    # - Grayscale image: (height, width) single channel
    img = cv2.imread(path, cv2.IMREAD_UNCHANGED)
    
    # Check if image was loaded successfully
    if img is None:
        raise FileNotFoundError(f"Could not load image: {path}")
    
    # Determine if input image is grayscale or color
    is_input_grayscale = len(img.shape) == 2 or img.shape[2] == 1
    
    # Resize to STM32's requested dimensions
    # cv2.resize expects (width, height) tuple
    img = cv2.resize(img, (width, height), interpolation=cv2.INTER_AREA)
    
    # Convert to requested format
    if format == IMAGE_FORMAT_GRAYSCALE:
        # STM32 wants grayscale format
        if is_input_grayscale:
            # Input is already grayscale, just ensure it's single channel
            if len(img.shape) == 3:
                img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
        else:
            # Input is color, convert to grayscale
            img = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
            
    elif format == IMAGE_FORMAT_RGB565:
        # STM32 wants RGB565 format
        if is_input_grayscale:
            # Input is grayscale, convert to color first, then to RGB565
            if len(img.shape) == 2:
                img = cv2.cvtColor(img, cv2.COLOR_GRAY2BGR)
            img = cv2.cvtColor(img, cv2.COLOR_BGR2BGR565)
        else:
            # Input is color, convert directly to RGB565
            img = cv2.cvtColor(img, cv2.COLOR_BGR2BGR565)
    
    # Convert numpy array to bytes and send over serial
    img = img.tobytes()
    __serial.write(img)
    
    return img  # Return the processed image array (before tobytes) for reference