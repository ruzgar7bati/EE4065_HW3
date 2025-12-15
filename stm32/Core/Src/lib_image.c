/*
 * lib_image.c
 */

#include "lib_image.h"

#define __LIB_IMAGE_CHECK_PARAM(param)				{if(param == 0) return IMAGE_ERROR;}

/**
  * @brief Initialize the image structure with required information
  * @param img     Pointer to image structure
  * @param pImg    Pointer to image buffer
  * @param height  height of the image
  * @param width   width of the image
  * @param format  Choose IMAGE_FORMAT_GRAYSCALE, IMAGE_FORMAT_RGB565, or IMAGE_FORMAT_RGB888
  * @retval 0 if successfully initialized
  */
int8_t LIB_IMAGE_InitStruct(IMAGE_HandleTypeDef * img, uint8_t *pImg, uint16_t height, uint16_t width, IMAGE_Format format)
{
	__LIB_IMAGE_CHECK_PARAM(img);
	__LIB_IMAGE_CHECK_PARAM(pImg);
	__LIB_IMAGE_CHECK_PARAM(format);
	__LIB_IMAGE_CHECK_PARAM(width);
	__LIB_IMAGE_CHECK_PARAM(height);
	img->format = format;
	img->height = height;
	img->width 	= width;
	img->pData 	= pImg;
	img->size 	= (uint32_t)img->format * (uint32_t)img->height * (uint32_t)img->width;
	return IMAGE_OK;
}
//Otsu's thresholding functions (Q1)
/**
  * @brief Calculate Otsu's threshold value for a grayscale image
  * @param img Pointer to grayscale image structure (must be IMAGE_FORMAT_GRAYSCALE)
  * @retval Calculated threshold value (0-255)
  * 
  * Algorithm:
  * 1. Build histogram of pixel intensities (0-255)
  * 2. Calculate probabilities for each intensity
  * 3. For each possible threshold, calculate inter-class variance
  * 4. Return threshold that maximizes variance
  */
uint8_t LIB_IMAGE_OtsuThreshold(IMAGE_HandleTypeDef * img)
{
	// Validate input: must be grayscale image
	if (img->format != IMAGE_FORMAT_GRAYSCALE)
	{
		return 0;  // Return 0 if invalid format
	}
	
	// ========================================================================
	// STEP 1: BUILD HISTOGRAM
	// ========================================================================
	// Count how many pixels have each intensity value (0-255)
	// Example: histogram[100] = 50 means 50 pixels have intensity 100
	uint32_t histogram[256] = {0};  // Initialize all counts to zero
	uint32_t totalPixels = img->width * img->height;  // Total number of pixels
	uint8_t *pData = img->pData;  // Pointer to image pixel data
	
	// Loop through every pixel in the image
	for (uint32_t i = 0; i < totalPixels; i++)
	{
		// Increment the count for this pixel's intensity value
		// pData[i] is the pixel intensity (0-255)
		histogram[pData[i]]++;
	}
	
	// ========================================================================
	// STEP 2: CALCULATE PROBABILITIES AND CUMULATIVE VALUES
	// ========================================================================
	// We need these to efficiently calculate class statistics for each threshold
	float prob[256];      // Probability of each intensity (0.0 to 1.0)
	float cumSum[256];    // Cumulative probability sum (ω₀ for threshold t)
	float cumMean[256];   // Cumulative weighted mean (for calculating μ₀)
	
	// Initialize first element (intensity 0)
	prob[0] = (float)histogram[0] / totalPixels;  // Probability = count / total
	cumSum[0] = prob[0];  // Cumulative sum starts with first probability
	cumMean[0] = 0.0f;    // Cumulative mean starts at 0
	
	// Calculate for all other intensities (1-255)
	for (int i = 1; i < 256; i++)
	{
		// Probability: what fraction of pixels have this intensity?
		prob[i] = (float)histogram[i] / totalPixels;
		
		// Cumulative sum: sum of all probabilities up to intensity i
		// This represents ω₀ (weight of class 0) if threshold = i
		cumSum[i] = cumSum[i-1] + prob[i];
		
		// Cumulative mean: weighted sum of intensities up to i
		// This helps us calculate the mean of class 0 efficiently
		cumMean[i] = cumMean[i-1] + (float)i * prob[i];
	}
	
	// ========================================================================
	// STEP 3: FIND THRESHOLD THAT MAXIMIZES INTER-CLASS VARIANCE
	// ========================================================================
	// Otsu's method: find threshold that best separates foreground and background
	// We do this by maximizing the variance between the two classes
	float maxVariance = 0.0f;  // Track the maximum variance found
	uint8_t bestThreshold = 0;  // The threshold value that gives max variance
	
	// Try every possible threshold value (0-255)
	for (int t = 0; t < 256; t++)
	{
		// Class 0: pixels with intensity <= threshold (background)
		// Class 1: pixels with intensity > threshold (foreground)
		
		// Weight of class 0: fraction of pixels in class 0
		float w0 = cumSum[t];  // Sum of probabilities from 0 to t
		
		// Weight of class 1: fraction of pixels in class 1
		float w1 = 1.0f - w0;  // Remaining pixels
		
		// Skip if one class is empty (no variance possible)
		if (w0 == 0.0f || w1 == 0.0f) continue;
		
		// Mean of class 0: average intensity of pixels <= threshold
		// cumMean[t] is the weighted sum, divide by weight to get mean
		float mean0 = (w0 > 0) ? cumMean[t] / w0 : 0.0f;
		
		// Mean of class 1: average intensity of pixels > threshold
		// Total mean minus class 0 mean, divided by class 1 weight
		float mean1 = (w1 > 0) ? (cumMean[255] - cumMean[t]) / w1 : 0.0f;
		
		// Inter-class variance: measures how well separated the two classes are
		// Formula: σ² = ω₀ × ω₁ × (μ₀ - μ₁)²
		// Higher variance = better separation = better threshold
		float variance = w0 * w1 * (mean0 - mean1) * (mean0 - mean1);
		
		// Keep track of the best threshold (highest variance)
		if (variance > maxVariance)
		{
			maxVariance = variance;
			bestThreshold = t;  // This threshold is the best so far
		}
	}
	
	// Return the optimal threshold value
	return bestThreshold;
}

/**
  * @brief Apply threshold to grayscale image, output binary image
  * @param imgIn Pointer to input grayscale image
  * @param imgOut Pointer to output binary image (must be initialized as GRAYSCALE)
  * @param threshold Threshold value (0-255)
  * @retval IMAGE_OK if successful, IMAGE_ERROR otherwise
  */
int8_t LIB_IMAGE_ApplyThreshold(IMAGE_HandleTypeDef * imgIn, IMAGE_HandleTypeDef * imgOut, uint8_t threshold)
{
	// Validate inputs: both must be grayscale format
	if (imgIn->format != IMAGE_FORMAT_GRAYSCALE || imgOut->format != IMAGE_FORMAT_GRAYSCALE)
	{
		return IMAGE_ERROR;
	}
	// Validate dimensions: input and output must be same size
	if (imgIn->width != imgOut->width || imgIn->height != imgOut->height)
	{
		return IMAGE_ERROR;
	}
	
	// Get total number of pixels and pointers to image data
	uint32_t totalPixels = imgIn->width * imgIn->height;
	uint8_t *pIn = imgIn->pData;   // Pointer to input image pixels
	uint8_t *pOut = imgOut->pData;  // Pointer to output image pixels
	
	// ========================================================================
	// APPLY THRESHOLD TO EACH PIXEL
	// ========================================================================
	// Convert grayscale image to binary (black and white only)
	// Rule: If pixel intensity > threshold → white (255), else → black (0)
	for (uint32_t i = 0; i < totalPixels; i++)
	{
		// Ternary operator: condition ? value_if_true : value_if_false
		// If input pixel > threshold, output is 255 (white), else 0 (black)
		pOut[i] = (pIn[i] > threshold) ? 255 : 0;
	}
	
	return IMAGE_OK;
}


//Color to grayscale conversion function (Q2)
/**
  * @brief Convert RGB565 color image to grayscale
  * @param imgIn Pointer to input color image (must be IMAGE_FORMAT_RGB565)
  * @param imgOut Pointer to output grayscale image (must be IMAGE_FORMAT_GRAYSCALE)
  * @retval IMAGE_OK if successful, IMAGE_ERROR otherwise
  * 
  * Algorithm:
  * 1. Extract R, G, B components from RGB565
  * 2. Convert to grayscale using: Gray = 0.299*R + 0.587*G + 0.114*B
  * 3. Store in output grayscale image
  */
int8_t LIB_IMAGE_ConvertToGrayscale(IMAGE_HandleTypeDef * imgIn, IMAGE_HandleTypeDef * imgOut)
{
	// Validate inputs: input must be RGB565, output must be grayscale
	if (imgIn->format != IMAGE_FORMAT_RGB565 || imgOut->format != IMAGE_FORMAT_GRAYSCALE)
	{
		return IMAGE_ERROR;
	}
	// Validate dimensions: input and output must be same size
	if (imgIn->width != imgOut->width || imgIn->height != imgOut->height)
	{
		return IMAGE_ERROR;
	}
	
	// RGB565 format: each pixel is 16 bits (2 bytes)
	// Format: RRRRR GGGGGG BBBBB (5 bits red, 6 bits green, 5 bits blue)
	uint16_t *pColorData = (uint16_t *)imgIn->pData;  // Cast to 16-bit pointer
	uint8_t *pGrayData = imgOut->pData;               // Output is 8-bit grayscale
	uint32_t totalPixels = imgIn->width * imgIn->height;
	
	// ========================================================================
	// CONVERT EACH COLOR PIXEL TO GRAYSCALE
	// ========================================================================
	for (uint32_t i = 0; i < totalPixels; i++)
	{
		// Get the 16-bit RGB565 pixel value
		uint16_t pixel = pColorData[i];
		
		// Extract Red component (5 bits, bits 11-15)
		// (pixel >> 11) shifts right to get top 5 bits
		// & 0x1F masks to keep only 5 bits (0x1F = 00011111 in binary)
		// << 3 scales from 5-bit (0-31) to 8-bit (0-255) range
		uint8_t r = ((pixel >> 11) & 0x1F) << 3;
		
		// Extract Green component (6 bits, bits 5-10)
		// (pixel >> 5) shifts right to get middle 6 bits
		// & 0x3F masks to keep only 6 bits (0x3F = 00111111 in binary)
		// << 2 scales from 6-bit (0-63) to 8-bit (0-255) range
		uint8_t g = ((pixel >> 5) & 0x3F) << 2;
		
		// Extract Blue component (5 bits, bits 0-4)
		// & 0x1F masks to keep only bottom 5 bits
		// << 3 scales from 5-bit (0-31) to 8-bit (0-255) range
		uint8_t b = (pixel & 0x1F) << 3;
		
		// ====================================================================
		// CONVERT TO GRAYSCALE USING LUMINANCE FORMULA
		// ====================================================================
		// Human eye is more sensitive to green, less to blue
		// Standard formula: Gray = 0.299*R + 0.587*G + 0.114*B
		// We use integer arithmetic to avoid floating point:
		// Gray = (299*R + 587*G + 114*B) / 1000
		// This gives us a single intensity value representing the color
		uint16_t gray = (299 * r + 587 * g + 114 * b) / 1000;
		
		// Store the grayscale value (0-255) in output image
		pGrayData[i] = (uint8_t)gray;
	}
	
	return IMAGE_OK;
}

  
//Morphological operations functions (Q3)
/**
  * @brief Apply erosion morphological operation
  * @param imgIn Pointer to input binary image
  * @param imgOut Pointer to output binary image
  * @param kernelSize Size of structuring element (typically 3)
  * @retval IMAGE_OK if successful
  */
int8_t LIB_IMAGE_Erosion(IMAGE_HandleTypeDef * imgIn, IMAGE_HandleTypeDef * imgOut, uint8_t kernelSize)
{
	// Validate inputs: both must be grayscale format
	if (imgIn->format != IMAGE_FORMAT_GRAYSCALE || imgOut->format != IMAGE_FORMAT_GRAYSCALE)
	{
		return IMAGE_ERROR;
	}
	// Validate dimensions: input and output must be same size
	if (imgIn->width != imgOut->width || imgIn->height != imgOut->height)
	{
		return IMAGE_ERROR;
	}
	
	uint8_t *pIn = imgIn->pData;   // Pointer to input image
	uint8_t *pOut = imgOut->pData; // Pointer to output image
	int halfKernel = kernelSize / 2;  // Half the kernel size (for 3x3, this is 1)
	
	// ========================================================================
	// EROSION: SHRINKS WHITE OBJECTS, EXPANDS BLACK BACKGROUND
	// ========================================================================
	// For each pixel, look at its neighborhood (kernel window)
	// Output = minimum value in the neighborhood
	// This makes white objects smaller and removes small white noise
	
	// Process each pixel in the image
	for (uint16_t y = 0; y < imgIn->height; y++)
	{
		for (uint16_t x = 0; x < imgIn->width; x++)
		{
			// ====================================================================
			// HANDLE BORDER PIXELS
			// ====================================================================
			// Pixels near the edge don't have a complete neighborhood
			// For simplicity, we just copy border pixels from input to output
			if (x < halfKernel || x >= imgIn->width - halfKernel ||
				y < halfKernel || y >= imgIn->height - halfKernel)
			{
				// Calculate pixel index: row * width + column
				pOut[y * imgIn->width + x] = pIn[y * imgIn->width + x];
				continue;  // Skip to next pixel
			}
			
			// ====================================================================
			// APPLY EROSION TO INTERIOR PIXELS
			// ====================================================================
			// Look at all pixels in the kernel neighborhood around (x, y)
			// Find the minimum value (darkest pixel) in the neighborhood
			uint8_t minVal = 255;  // Start with maximum (white)
			
			// Scan through kernel window (e.g., 3x3 = -1 to +1 in both directions)
			for (int ky = -halfKernel; ky <= halfKernel; ky++)
			{
				for (int kx = -halfKernel; kx <= halfKernel; kx++)
				{
					// Get pixel value at this position in the neighborhood
					// (y + ky) is the row, (x + kx) is the column
					uint8_t val = pIn[(y + ky) * imgIn->width + (x + kx)];
					
					// Keep track of the minimum (darkest) value found
					if (val < minVal)
					{
						minVal = val;
					}
				}
			}
			
			// Output pixel = minimum value in neighborhood
			// This shrinks white objects (if any neighbor is black, output becomes black)
			pOut[y * imgIn->width + x] = minVal;
		}
	}
	
	return IMAGE_OK;
}
  
  /**
	* @brief Apply dilation morphological operation
	* @param imgIn Pointer to input binary image
	* @param imgOut Pointer to output binary image
	* @param kernelSize Size of structuring element (typically 3)
	* @retval IMAGE_OK if successful
	*/
int8_t LIB_IMAGE_Dilation(IMAGE_HandleTypeDef * imgIn, IMAGE_HandleTypeDef * imgOut, uint8_t kernelSize)
{
	// Validate inputs: both must be grayscale format
	if (imgIn->format != IMAGE_FORMAT_GRAYSCALE || imgOut->format != IMAGE_FORMAT_GRAYSCALE)
	{
		return IMAGE_ERROR;
	}
	// Validate dimensions: input and output must be same size
	if (imgIn->width != imgOut->width || imgIn->height != imgOut->height)
	{
		return IMAGE_ERROR;
	}
	
	uint8_t *pIn = imgIn->pData;   // Pointer to input image
	uint8_t *pOut = imgOut->pData; // Pointer to output image
	int halfKernel = kernelSize / 2;  // Half the kernel size (for 3x3, this is 1)
	
	// ========================================================================
	// DILATION: EXPANDS WHITE OBJECTS, SHRINKS BLACK BACKGROUND
	// ========================================================================
	// For each pixel, look at its neighborhood (kernel window)
	// Output = maximum value in the neighborhood
	// This makes white objects larger and fills small black holes
	
	// Process each pixel in the image
	for (uint16_t y = 0; y < imgIn->height; y++)
	{
		for (uint16_t x = 0; x < imgIn->width; x++)
		{
			// ====================================================================
			// HANDLE BORDER PIXELS
			// ====================================================================
			// Pixels near the edge don't have a complete neighborhood
			// For simplicity, we just copy border pixels from input to output
			if (x < halfKernel || x >= imgIn->width - halfKernel ||
				y < halfKernel || y >= imgIn->height - halfKernel)
			{
				// Calculate pixel index: row * width + column
				pOut[y * imgIn->width + x] = pIn[y * imgIn->width + x];
				continue;  // Skip to next pixel
			}
			
			// ====================================================================
			// APPLY DILATION TO INTERIOR PIXELS
			// ====================================================================
			// Look at all pixels in the kernel neighborhood around (x, y)
			// Find the maximum value (brightest pixel) in the neighborhood
			uint8_t maxVal = 0;  // Start with minimum (black)
			
			// Scan through kernel window (e.g., 3x3 = -1 to +1 in both directions)
			for (int ky = -halfKernel; ky <= halfKernel; ky++)
			{
				for (int kx = -halfKernel; kx <= halfKernel; kx++)
				{
					// Get pixel value at this position in the neighborhood
					// (y + ky) is the row, (x + kx) is the column
					uint8_t val = pIn[(y + ky) * imgIn->width + (x + kx)];
					
					// Keep track of the maximum (brightest) value found
					if (val > maxVal)
					{
						maxVal = val;
					}
				}
			}
			
			// Output pixel = maximum value in neighborhood
			// This expands white objects (if any neighbor is white, output becomes white)
			pOut[y * imgIn->width + x] = maxVal;
		}
	}
	
	return IMAGE_OK;
}
  
  /**
	* @brief Apply opening (erosion followed by dilation)
	* @param imgIn Pointer to input binary image
	* @param imgOut Pointer to output binary image
	* @param kernelSize Size of structuring element (typically 3)
	* @retval IMAGE_OK if successful
	*/
int8_t LIB_IMAGE_Opening(IMAGE_HandleTypeDef * imgIn, IMAGE_HandleTypeDef * imgOut, uint8_t kernelSize)
{
	// ========================================================================
	// OPENING = EROSION FOLLOWED BY DILATION
	// ========================================================================
	// Opening removes small white objects and smooths object boundaries
	// It's useful for removing noise and separating touching objects
	// 
	// Process:
	//   1. Erosion: Shrinks objects (removes small protrusions)
	//   2. Dilation: Expands objects back (but small objects stay removed)
	
	// We need a temporary buffer to store the intermediate result (after erosion)
	// Using a static buffer to avoid dynamic allocation (limited to 128x128 images)
	static uint8_t tempBuffer[128*128];
	IMAGE_HandleTypeDef imgTemp;
	
	// Check if image fits in our temporary buffer
	if (imgIn->width > 128 || imgIn->height > 128)
	{
		return IMAGE_ERROR;  // Image too large for temporary buffer
	}
	
	// Initialize temporary image structure
	LIB_IMAGE_InitStruct(&imgTemp, tempBuffer, imgIn->height, imgIn->width, IMAGE_FORMAT_GRAYSCALE);
	
	// ========================================================================
	// STEP 1: APPLY EROSION
	// ========================================================================
	// Shrink white objects, remove small white noise
	// Result stored in tempBuffer
	if (LIB_IMAGE_Erosion(imgIn, &imgTemp, kernelSize) != IMAGE_OK)
	{
		return IMAGE_ERROR;
	}
	
	// ========================================================================
	// STEP 2: APPLY DILATION ON ERODED RESULT
	// ========================================================================
	// Expand objects back, but small objects that were removed stay removed
	// Final result stored in imgOut
	return LIB_IMAGE_Dilation(&imgTemp, imgOut, kernelSize);
}
  
  /**
	* @brief Apply closing (dilation followed by erosion)
	* @param imgIn Pointer to input binary image
	* @param imgOut Pointer to output binary image
	* @param kernelSize Size of structuring element (typically 3)
	* @retval IMAGE_OK if successful
	*/
int8_t LIB_IMAGE_Closing(IMAGE_HandleTypeDef * imgIn, IMAGE_HandleTypeDef * imgOut, uint8_t kernelSize)
{
	// ========================================================================
	// CLOSING = DILATION FOLLOWED BY EROSION
	// ========================================================================
	// Closing fills small black holes and smooths object boundaries
	// It's useful for connecting nearby objects and filling gaps
	// 
	// Process:
	//   1. Dilation: Expands objects (fills small holes)
	//   2. Erosion: Shrinks objects back (but holes stay filled)
	
	// We need a temporary buffer to store the intermediate result (after dilation)
	// Using a static buffer to avoid dynamic allocation (limited to 128x128 images)
	static uint8_t tempBuffer[128*128];
	IMAGE_HandleTypeDef imgTemp;
	
	// Check if image fits in our temporary buffer
	if (imgIn->width > 128 || imgIn->height > 128)
	{
		return IMAGE_ERROR;  // Image too large for temporary buffer
	}
	
	// Initialize temporary image structure
	LIB_IMAGE_InitStruct(&imgTemp, tempBuffer, imgIn->height, imgIn->width, IMAGE_FORMAT_GRAYSCALE);
	
	// ========================================================================
	// STEP 1: APPLY DILATION
	// ========================================================================
	// Expand white objects, fill small black holes
	// Result stored in tempBuffer
	if (LIB_IMAGE_Dilation(imgIn, &imgTemp, kernelSize) != IMAGE_OK)
	{
		return IMAGE_ERROR;
	}
	
	// ========================================================================
	// STEP 2: APPLY EROSION ON DILATED RESULT
	// ========================================================================
	// Shrink objects back, but holes that were filled stay filled
	// Final result stored in imgOut
	return LIB_IMAGE_Erosion(&imgTemp, imgOut, kernelSize);
}