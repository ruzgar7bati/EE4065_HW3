/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lib_image.h"
#include "lib_serialimage.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// ============================================================================
// IMAGE BUFFERS - OPTIMIZED (REUSED ACROSS QUESTIONS)
// ============================================================================
// Since questions run sequentially, we can reuse buffers to save memory
// Total memory: 32KB (color) + 16KB (grayscale) + 16KB (binary) + 16KB (temp) = 80KB

volatile uint8_t pImageColor[128*128*2];     // Q2: Input color RGB565 (32KB)
volatile uint8_t pImageGray[128*128*1];      // Q1: Input grayscale, Q2: Converted grayscale (16KB) - REUSED
volatile uint8_t pImageBinary[128*128*1];    // Q1: Binary output, Q3: Binary input, Q3: Results (16KB) - REUSED
volatile uint8_t pImageTemp[128*128*1];     // Q3: Temporary for opening/closing (16KB)

// ============================================================================
// IMAGE STRUCTURE HANDLES - REUSED ACROSS QUESTIONS
// ============================================================================
IMAGE_HandleTypeDef imgColor;    // Q2: Input color image
IMAGE_HandleTypeDef imgGray;     // Q1: Input grayscale, Q2: Converted grayscale (reused)
IMAGE_HandleTypeDef imgBinary;   // Q1: Binary output, Q2: Binary output, Q3: Input/Output (reused)
IMAGE_HandleTypeDef imgTemp;     // Q3: Temporary for opening/closing operations
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/
  
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  // Initialize image structures (buffers will be reused across questions)
  // These will be re-initialized with different formats as needed
  LIB_IMAGE_InitStruct(&imgColor, (uint8_t*)pImageColor, 128, 128, IMAGE_FORMAT_RGB565);
  LIB_IMAGE_InitStruct(&imgGray, (uint8_t*)pImageGray, 128, 128, IMAGE_FORMAT_GRAYSCALE);
  LIB_IMAGE_InitStruct(&imgBinary, (uint8_t*)pImageBinary, 128, 128, IMAGE_FORMAT_GRAYSCALE);
  LIB_IMAGE_InitStruct(&imgTemp, (uint8_t*)pImageTemp, 128, 128, IMAGE_FORMAT_GRAYSCALE);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    // ============================================================
    // Q1: Otsu's Thresholding on Grayscale Images
    // ============================================================
    // CYCLE 1: Send image → Receive result
    if (LIB_SERIAL_IMG_Receive(&imgGray) == SERIAL_OK)
    {
        // Calculate Otsu threshold and apply
        uint8_t threshold = LIB_IMAGE_OtsuThreshold(&imgGray);
        LIB_IMAGE_ApplyThreshold(&imgGray, &imgBinary, threshold);
        
        // Send binary result back to PC (cycle 1 complete)
        LIB_SERIAL_IMG_Transmit(&imgBinary);
    }
    
    // ============================================================
    // Q2: Otsu's Thresholding on Color Images
    // ============================================================
    // CYCLE 2: Send image → Receive result
    if (LIB_SERIAL_IMG_Receive(&imgColor) == SERIAL_OK)
    {
        // Convert color to grayscale (reuse imgGray buffer)
        LIB_IMAGE_ConvertToGrayscale(&imgColor, &imgGray);
        
        // Calculate Otsu threshold and apply (reuse imgBinary buffer)
        uint8_t threshold = LIB_IMAGE_OtsuThreshold(&imgGray);
        LIB_IMAGE_ApplyThreshold(&imgGray, &imgBinary, threshold);
        
        // Send binary result back to PC (cycle 2 complete)
        LIB_SERIAL_IMG_Transmit(&imgBinary);
    }
    
    // ============================================================
    // Q3: Morphological Operations (each operation is a separate cycle)
    // ============================================================
    // CYCLE 3: Erosion - Send binary → Receive erosion result
    if (LIB_SERIAL_IMG_Receive(&imgBinary) == SERIAL_OK)
    {
        LIB_IMAGE_Erosion(&imgBinary, &imgTemp, 3);  // Use temp as output
        LIB_SERIAL_IMG_Transmit(&imgTemp);  // Send erosion result (cycle 3 complete)
    }
    
    // CYCLE 4: Dilation - Send binary → Receive dilation result
    if (LIB_SERIAL_IMG_Receive(&imgBinary) == SERIAL_OK)
    {
        LIB_IMAGE_Dilation(&imgBinary, &imgTemp, 3);  // Use temp as output
        LIB_SERIAL_IMG_Transmit(&imgTemp);  // Send dilation result (cycle 4 complete)
    }
    
    // CYCLE 5: Opening - Send binary → Receive opening result
    if (LIB_SERIAL_IMG_Receive(&imgBinary) == SERIAL_OK)
    {
        // Opening = erosion then dilation
        LIB_IMAGE_Erosion(&imgBinary, &imgTemp, 3);
        LIB_IMAGE_Dilation(&imgTemp, &imgBinary, 3);  // Reuse imgBinary as output
        LIB_SERIAL_IMG_Transmit(&imgBinary);  // Send opening result (cycle 5 complete)
    }
    
    // CYCLE 6: Closing - Send binary → Receive closing result
    if (LIB_SERIAL_IMG_Receive(&imgBinary) == SERIAL_OK)
    {
        // Closing = dilation then erosion
        LIB_IMAGE_Dilation(&imgBinary, &imgTemp, 3);
        LIB_IMAGE_Erosion(&imgTemp, &imgBinary, 3);  // Reuse imgBinary as output
        LIB_SERIAL_IMG_Transmit(&imgBinary);  // Send closing result (cycle 6 complete)
    }
    
    // ============================================================
    // ALL CYCLES COMPLETE - Enter idle state (wait for reset)
    // ============================================================
    while(1)
    {
        HAL_Delay(1000);  // Wait until board is reset
    }
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 2000000;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

