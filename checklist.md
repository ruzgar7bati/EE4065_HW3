# Homework 3 - Implementation Checklist

## Task 1: C - Grayscale Otsu Thresholding
- [ ] Add Otsu threshold calculation function to `lib_image.c`
  - [ ] Function: `LIB_IMAGE_OtsuThreshold(IMAGE_HandleTypeDef * img)`
  - [ ] Build histogram (0-255)
  - [ ] Calculate probabilities
  - [ ] Find threshold that maximizes inter-class variance
  - [ ] Return threshold value (uint8_t)
- [ ] Add function declaration to `lib_image.h`
- [ ] Add threshold application function to `lib_image.c`
  - [ ] Function: `LIB_IMAGE_ApplyThreshold(IMAGE_HandleTypeDef * imgIn, IMAGE_HandleTypeDef * imgOut, uint8_t threshold)`
  - [ ] Convert grayscale to binary (0 or 255)
  - [ ] Add function declaration to `lib_image.h`
- [ ] Modify `main.c` for grayscale processing
  - [ ] Change buffer from RGB565 to GRAYSCALE format
  - [ ] Add separate buffer for binary output image
  - [ ] Initialize image structures for grayscale input and binary output
  - [ ] In main loop: receive grayscale → calculate Otsu → apply threshold → send binary result

## Task 2: Python - Visualize Grayscale Results
- [ ] Modify `py_image.py` to handle grayscale visualization
  - [ ] Import matplotlib (if not already)
  - [ ] When receiving binary image from STM32, display it properly
  - [ ] Show original grayscale image (before sending) and result side-by-side
  - [ ] Save both original and processed images
- [ ] Test grayscale send/receive flow
  - [ ] Verify STM32 requests GRAYSCALE format
  - [ ] Verify Python converts mandrill.tiff to grayscale
  - [ ] Verify binary result is received and displayed correctly

## Task 3: C - Erosion
- [ ] Add erosion function to `lib_image.c`
  - [ ] Function: `LIB_IMAGE_Erosion(IMAGE_HandleTypeDef * imgIn, IMAGE_HandleTypeDef * imgOut, uint8_t kernelSize)`
  - [ ] Implement 3x3 kernel (or configurable size)
  - [ ] For each pixel: output = minimum value in kernel neighborhood
  - [ ] Handle border pixels (copy from input)
  - [ ] Add function declaration to `lib_image.h`
- [ ] Modify `main.c` to apply erosion
  - [ ] Add buffer for eroded image result
  - [ ] Initialize eroded image structure
  - [ ] Apply erosion to binary image after thresholding
  - [ ] Option to send eroded result back to PC

## Task 4: C - Dilation
- [ ] Add dilation function to `lib_image.c`
  - [ ] Function: `LIB_IMAGE_Dilation(IMAGE_HandleTypeDef * imgIn, IMAGE_HandleTypeDef * imgOut, uint8_t kernelSize)`
  - [ ] Implement 3x3 kernel (or configurable size)
  - [ ] For each pixel: output = maximum value in kernel neighborhood
  - [ ] Handle border pixels (copy from input)
  - [ ] Add function declaration to `lib_image.h`
- [ ] Modify `main.c` to apply dilation
  - [ ] Add buffer for dilated image result
  - [ ] Initialize dilated image structure
  - [ ] Apply dilation to binary image after thresholding
  - [ ] Option to send dilated result back to PC

## Task 5: Integration & Testing
- [ ] Test complete flow: PC → STM32 (grayscale) → Otsu → Binary → STM32 → PC
- [ ] Test erosion: PC → STM32 → Otsu → Erosion → STM32 → PC
- [ ] Test dilation: PC → STM32 → Otsu → Dilation → STM32 → PC
- [ ] Verify all images are displayed correctly in Python
- [ ] Verify memory usage is within STM32 limits (check buffer sizes)
- [ ] Test with different images if possible

## Task 6: Report - Otsu Theory + Implementation
- [ ] Write Otsu's method theory section
  - [ ] Explain what Otsu's method does
  - [ ] Explain inter-class variance concept
  - [ ] Explain why it works (mathematical reasoning)
- [ ] Write implementation section
  - [ ] Explain function structure
  - [ ] Explain histogram calculation
  - [ ] Explain threshold selection algorithm
  - [ ] Include code snippets
- [ ] Add results section
  - [ ] Include original grayscale image
  - [ ] Include thresholded binary image
  - [ ] Report the calculated threshold value
  - [ ] Discuss results

## Task 7: Report - Erosion/Dilation Explanation + Results
- [ ] Write morphological operations theory section
  - [ ] Explain what erosion does (shrinks objects)
  - [ ] Explain what dilation does (expands objects)
  - [ ] Explain structuring elements (kernels)
  - [ ] Explain applications
- [ ] Write implementation section
  - [ ] Explain kernel-based approach
  - [ ] Explain min/max operations
  - [ ] Explain border handling
  - [ ] Include code snippets
- [ ] Add results section
  - [ ] Include original binary image (from Otsu)
  - [ ] Include eroded image
  - [ ] Include dilated image
  - [ ] Compare and discuss differences
  - [ ] Show side-by-side comparison

## Code Files to Modify/Create:
- [ ] `stm32/Core/Src/lib_image.c` - Add processing functions
- [ ] `stm32/Core/Inc/lib_image.h` - Add function declarations
- [ ] `stm32/Core/Src/main.c` - Modify for grayscale processing workflow
- [ ] `python/py_image.py` - Add visualization (optional enhancement)
- [ ] Report document (separate file)

## Memory Check:
- [ ] Verify buffer sizes:
  - [ ] Grayscale input: 128×128×1 = 16,384 bytes (16 KB)
  - [ ] Binary output: 128×128×1 = 16,384 bytes (16 KB)
  - [ ] Eroded result: 128×128×1 = 16,384 bytes (16 KB)
  - [ ] Dilated result: 128×128×1 = 16,384 bytes (16 KB)
  - [ ] Total: 64 KB (within 128 KB SRAM limit ✓)

## Notes:
- Current buffer in main.c: `pImage[128*128*2]` = 32 KB (RGB565)
- Need to change to grayscale buffers: `pImageGray[128*128*1]` = 16 KB each
- STM32 currently requests RGB565 format - need to change to GRAYSCALE
- Python side already supports grayscale conversion (in py_serialimg.py)