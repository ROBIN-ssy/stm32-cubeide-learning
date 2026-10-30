/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  * 源代码控制LED亮灭，LED位于B0 A7 A6
  * B11和B10为通信接口
  * 烧录后自动开启蓝牙广播。手机APP为BLEDemo
  * R1	52 31
  *R0	52 30
  *G1	47 31
  *G0	47 30
  *B1	42 31
  *B0	42 30
  *仅可使用ASCII Hex发送信息
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
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
UART_HandleTypeDef huart3;
DMA_HandleTypeDef hdma_usart3_tx;
DMA_HandleTypeDef hdma_usart3_rx;

/* USER CODE BEGIN PV */
uint8_t receiveData[50];
uint8_t txBuf[50];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */
static void BC7701_SendNotify(uint8_t *data, uint8_t len);
static void BC7701_StartAdvertising(void);
static void ProcessPhoneData(uint8_t *data, uint8_t len);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*
 * BC7701 协议说明：
 * 手机 -> 0xFFF2(Write Without Response) -> BC7701 -> UART 转发给 STM32
 *   收到的包格式: 78 [Len] [EvtStatus] [F2 FF] [手机原始数据...]
 *   Header=0x78, Length=后续总字节数, Type=0xFFF2(LSB first -> F2 FF)
 *
 * STM32 -> 0xFFF1(Notify) -> BC7701 -> 推送给手机
 *   发送的包格式: 77 [Len] 20 F1 FF [数据...]
 *   CmdFlag=0x20 表示触发 Notify, Type=0xFFF1(LSB first -> F1 FF)
 *
 * 开启广播: 77 04 00 07 00 01
 */

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if (huart == &huart3)
	{
		/*
		 * 检查是否是 BC7701 转发的手机数据包
		 * 格式: 78 [Len] [EvtStatus] [F2 FF] [手机原始数据...]
		 */
		if (Size >= 5 && receiveData[0] == 0x78)
		{
			uint8_t evtStatus = receiveData[2];
			uint8_t typeLow   = receiveData[3];
			uint8_t typeHigh  = receiveData[4];

			/* 只处理 0xFFF2 (Write Without Response) 的数据包 */
			if (evtStatus == 0x00 && typeLow == 0xF2 && typeHigh == 0xFF)
			{
				/* 手机原始数据从 receiveData[5] 开始 */
				uint8_t *phoneData = &receiveData[5];
				uint8_t  phoneLen   = Size - 5;

				if (phoneLen > 0)
				{
					ProcessPhoneData(phoneData, phoneLen);
				}
			}
		}

		/* 重新启动 DMA 接收 */
		HAL_UARTEx_ReceiveToIdle_DMA(&huart3, receiveData, sizeof(receiveData));
		__HAL_DMA_DISABLE_IT(&hdma_usart3_rx, DMA_IT_HT);
	}
}

/*
 * 处理手机发来的数据
 * 手机发送 R1/G1/B1 -> 对应 LED 亮
 * 手机发送 R0/G0/B0 -> 对应 LED 灭
 * 然后通过 0xFFF1 Notify 把原始数据 Echo 回手机
 */
static void ProcessPhoneData(uint8_t *data, uint8_t len)
{
	/* --- LED 控制 --- */
	if (len >= 2)
	{
		if (data[0] == 'R')
		{
			if (data[1] == '1')
				HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);
			else if (data[1] == '0')
				HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);
		}
		else if (data[0] == 'G')
		{
			if (data[1] == '1')
				HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);
			else if (data[1] == '0')
				HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET);
		}
		else if (data[0] == 'B')
		{
			if (data[1] == '1')
				HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET);
			else if (data[1] == '0')
				HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET);
		}
	}

	/* --- Echo 回复：通过 0xFFF1 Notify 把原始数据发回手机 --- */
	BC7701_SendNotify(data, len);
}

/*
 * 通过 BC7701 的 0xFFF1 通道发送 Notify 数据给手机
 * 命令格式: 77 [Len] 20 F1 FF [数据...]
 *   Header   = 0x77
 *   Length   = 1(CmdFlag) + 2(Type) + N(数据) = 3 + len
 *   CmdFlag  = 0x20 (b[7:4]=2 -> 触发 Notify)
 *   Type     = 0xFFF1 (LSB first -> F1 FF)
 */
static void BC7701_SendNotify(uint8_t *data, uint8_t len)
{
	if (len > 40)
		len = 40;  /* 防止缓冲区溢出 */

	txBuf[0] = 0x77;
	txBuf[1] = 3 + len;       /* Length = CmdFlag + Type + Value */
	txBuf[2] = 0x20;          /* CmdFlag: 触发 Notify */
	txBuf[3] = 0xF1;          /* Type low byte */
	txBuf[4] = 0xFF;          /* Type high byte */
	memcpy(&txBuf[5], data, len);

	/* 用阻塞模式发送，避免在中断回调中忙等 DMA 造成死锁 */
	HAL_UART_Transmit(&huart3, txBuf, 5 + len, 100);
}

/*
 * 上电后向 BC7701 发送开启广播指令
 * 命令: 77 04 00 07 00 01
 *   Header  = 0x77
 *   Length  = 0x04 (CmdFlag + Type + Value = 1+2+1)
 *   CmdFlag = 0x00
 *   Type    = 0x0007 (LSB first -> 07 00)
 *   Value   = 0x01 (开启广播)
 */
static void BC7701_StartAdvertising(void)
{
	uint8_t advCmd[] = {0x77, 0x04, 0x00, 0x07, 0x00, 0x01};
	HAL_UART_Transmit(&huart3, advCmd, sizeof(advCmd), 100);
}
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
  MX_DMA_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_UARTEx_ReceiveToIdle_DMA(&huart3, receiveData, sizeof(receiveData));
  __HAL_DMA_DISABLE_IT(&hdma_usart3_rx, DMA_IT_HT);

  /* 等待 BC7701 上电启动完成（至少 60ms） */
  HAL_Delay(100);

  /* 开启 BC7701 广播 */
  BC7701_StartAdvertising();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief USART3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 115200;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */

  /* USER CODE END USART3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel2_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel2_IRQn);
  /* DMA1_Channel3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel3_IRQn);

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
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LED_BLUE_Pin|LED_GREEN_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LED_BLUE_Pin LED_GREEN_Pin */
  GPIO_InitStruct.Pin = LED_BLUE_Pin|LED_GREEN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_RED_Pin */
  GPIO_InitStruct.Pin = LED_RED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_RED_GPIO_Port, &GPIO_InitStruct);

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
