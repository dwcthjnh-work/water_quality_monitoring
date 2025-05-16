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
#include "stdio.h"
#include "DHT.c"
#include "DS18B20.h"
#include "i2c-lcd.h"
#include "delay_timer.h"
#include "string.h"
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
ADC_HandleTypeDef hadc1;
ADC_HandleTypeDef hadc2;

I2C_HandleTypeDef hi2c1;
I2C_HandleTypeDef hi2c2;

TIM_HandleTypeDef htim3;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */
float tempDS18B20, tempDHT, humidity;
char buffer[100];
char msg[20];
DS18B20_Name DS1;
float TempWater;
uint16_t adc_value1 = 0;
float Vin1 = 0;
uint16_t adc_value2 = 0;
float Vin2 = 0;
uint16_t adc_value3 = 0;
float Vin3 = 0;
uint16_t adc_value4 = 0;
float Vin4 = 0;
float tdsValue;
float ntuValue;
float phValue;
float level;
int countdown;
volatile uint8_t current_page = 0; 
volatile uint8_t update_lcd = 1;   
float phSum = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2C2_Init(void);
static void MX_TIM3_Init(void);
static void MX_ADC2_Init(void);
/* USER CODE BEGIN PFP */
void send_data_UART(char *msg) {
    HAL_UART_Transmit(&huart1, (uint8_t *)msg, strlen(msg), HAL_MAX_DELAY);
}
void lcd_send_float(const char* label, float value) {
    static char buf[32];
    snprintf(buf, sizeof(buf), "%s: %.2f", label, value);
    lcd_send_string(buf);
}
float calib_ph(float voltage) {
    if (voltage >= 2.20 && voltage <= 2.52) {
        // Ðo?n t? pH 4.01 d?n 6.86
        float v1 = 2.52, pH1 = 4.01;
        float v2 = 2.20, pH2 = 6.86;
        return pH1 + (voltage - v1) * (pH2 - pH1) / (v2 - v1);
    } else if (voltage >= 1.93 && voltage < 2.20) {
        // Ðo?n t? pH 6.86 d?n 9.18
        float v1 = 2.20, pH1 = 6.86;
        float v2 = 1.93, pH2 = 9.18;
        return pH1 + (voltage - v1) * (pH2 - pH1) / (v2 - v1);
    } else if (voltage > 2.52) {
        return 4.01; 
    } else {
        return 9.18; 
    }
}

void show_page(uint8_t page) {
    char line1[17], line2[17];
    lcd_clear();
    
    if (page == 0) {
        snprintf(line1, sizeof(line1), "TempAir:  %.1f", tempDHT);
        snprintf(line2, sizeof(line2), "Humidity: %.1f", humidity);
    } else if (page == 1) {
        snprintf(line1, sizeof(line1), "TempWater:%.1f", TempWater);
        snprintf(line2, sizeof(line2), "TDS:      %.1f", tdsValue);
    } else if (page == 2) {
        snprintf(line1, sizeof(line1), "Level:    %.1f", level);
        snprintf(line2, sizeof(line2), "pH:       %.1f", phValue);
    } else if (page == 3) {
        snprintf(line1, sizeof(line1), "NTU:      %.1f", ntuValue);
        snprintf(line2, sizeof(line2), "Time left:%2ds", countdown);
    }

    lcd_put_cur(0, 0);
    lcd_send_string(line1);
    lcd_put_cur(1, 0);
    lcd_send_string(line2);
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	  static uint8_t relay1_state = 0;  
    static uint8_t relay2_state = 0; 
	  static uint8_t relay3_state = 0;  
    static uint8_t relay4_state = 0;  
    if (GPIO_Pin == GPIO_PIN_12) {
        static uint32_t last_tick = 0;
        if (HAL_GetTick() - last_tick > 400) {
            last_tick = HAL_GetTick();
            current_page = (current_page + 1) % 4;
            show_page(current_page);
        }
    }
		if (GPIO_Pin == GPIO_PIN_5) {
        static uint32_t last_tick = 0;
        if (HAL_GetTick() - last_tick > 200) {
            relay1_state = !relay1_state;  
            snprintf(msg, sizeof(msg), "relay1: %d\n", relay1_state);  
            send_data_UART(msg);
        }
    }
		if (GPIO_Pin == GPIO_PIN_6) {
        static uint32_t last_tick = 0;
        if (HAL_GetTick() - last_tick > 200) {
            relay2_state = !relay2_state;  
            snprintf(msg, sizeof(msg), "relay2: %d\n", relay2_state);  
            send_data_UART(msg);
        }
    }
		if (GPIO_Pin == GPIO_PIN_7) {
        static uint32_t last_tick = 0;
        if (HAL_GetTick() - last_tick > 200) {
            relay3_state = !relay3_state;  
            snprintf(msg, sizeof(msg), "relay3: %d\n", relay3_state); 
            send_data_UART(msg);
        }
    }
		if (GPIO_Pin == GPIO_PIN_0) {
        static uint32_t last_tick = 0;
        if (HAL_GetTick() - last_tick > 200) {
            relay4_state = !relay4_state;  
            snprintf(msg, sizeof(msg), "relay4: %d\n", relay4_state);  
            send_data_UART(msg);
        }
    }
}

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
DHT_sensor sensor = {
    .DHT_Port = GPIOB,
    .DHT_Pin = GPIO_PIN_14,
    .pullUp = GPIO_PULLUP,  
    .type = DHT11           
};
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
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  MX_I2C2_Init();
  MX_TIM3_Init();
  MX_ADC2_Init();
	lcd_init();
  /* USER CODE BEGIN 2 */
	DS18B20_Init(&DS1, &htim3, GPIOB,  GPIO_PIN_15);
	DHT_data dhtData;
	//Ds18B20_t ds18b20;
	//DS18B20_init(&ds18b20, &huart3, DS18B20_12BITS);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    HAL_ADC_Start(&hadc1);
		for (int i = 0; i < 3; i++) {
				HAL_ADC_PollForConversion(&hadc1, 100);
				uint16_t value = HAL_ADC_GetValue(&hadc1);
				switch (i) {
						case 0:
								adc_value1 = value;
								break;
						case 1:
								adc_value2 = value;
								break;
						case 2:
								adc_value3 = value;
								break;
				}
		}
		Vin1 = (float)(adc_value1/4095.00)*3.3;
				tdsValue = (133.42 * Vin1 * Vin1 * Vin1
											- 255.86 * Vin1 * Vin1
											+ 857.39 * Vin1) * 0.5;
				sprintf(buffer, "TDS: %.2f\r\n", tdsValue);
				send_data_UART(buffer);
		HAL_Delay(100); 
		Vin2 = (float)(adc_value2/4095.00)*3.3;
				ntuValue = (1.85- Vin2) * (500.0 / 2.2);	
				if (ntuValue < 0.1) {
						ntuValue = 0.1;
				}
				sprintf(buffer, "NTU: %.2f\r\n", ntuValue);
				send_data_UART(buffer);
				HAL_Delay(100); 

		Vin3 = (float)(adc_value3/4095.00)*3.3;
				if (Vin3 <= 0.05f) {
						level = 0; 
				} else if (Vin3 > 0.05f && Vin3 <= 1.25f) {
						level = 1;  
				} else if (Vin3 > 1.25f && Vin3 <= 1.45f) {
						level = 2; 
				} else if (Vin3 > 1.45f && Vin3 <= 2.20f) {
						level = 3;  
				} else {
						level = 4;  
				}
		sprintf(buffer, "Level: %.2f\r\n", level);
		send_data_UART(buffer);
		HAL_ADC_Stop(&hadc1);

		for (int i = 0; i < 10; i++) {
				HAL_ADC_Start(&hadc2);
				HAL_ADC_PollForConversion(&hadc2, 300);
				adc_value4 = HAL_ADC_GetValue(&hadc2);
				HAL_ADC_Stop(&hadc2);

				Vin4 = (float)adc_value4 / 4095.0 * 3.3;
				phValue = calib_ph(Vin4);
				phSum += phValue;

				HAL_Delay(50);  
		}
		float phAverage = phSum / 10.0;
		sprintf(buffer, "pHValue: %.2f\r\n", phValue);
		send_data_UART(buffer);
		
		TempWater = DS18B20_ReadTemp(&DS1);
		sprintf(buffer, "TempWater: %.2f\r\n", TempWater);		
		send_data_UART(buffer);     
		dhtData = DHT_getData(&sensor);
		humidity = dhtData.hum;
		sprintf(buffer, "Humidity: %.2f\r\n", humidity);
		send_data_UART(buffer);
		HAL_Delay(100);
		tempDHT = dhtData.temp;
		sprintf(buffer, "DHT Temp: %.2f\r\n", tempDHT);
		send_data_UART(buffer); 
		for (int i = 15; i >= 0; i--) {
			countdown = i;
			HAL_Delay(1000);  
		 show_page(current_page);
		}		
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 3;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief ADC2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC2_Init(void)
{

  /* USER CODE BEGIN ADC2_Init 0 */

  /* USER CODE END ADC2_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC2_Init 1 */

  /* USER CODE END ADC2_Init 1 */

  /** Common config
  */
  hadc2.Instance = ADC2;
  hadc2.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc2.Init.ContinuousConvMode = DISABLE;
  hadc2.Init.DiscontinuousConvMode = DISABLE;
  hadc2.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc2.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc2.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc2) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc2, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC2_Init 2 */

  /* USER CODE END ADC2_Init 2 */

}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2C2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C2_Init(void)
{

  /* USER CODE BEGIN I2C2_Init 0 */

  /* USER CODE END I2C2_Init 0 */

  /* USER CODE BEGIN I2C2_Init 1 */

  /* USER CODE END I2C2_Init 1 */
  hi2c2.Instance = I2C2;
  hi2c2.Init.ClockSpeed = 100000;
  hi2c2.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c2.Init.OwnAddress1 = 0;
  hi2c2.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c2.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c2.Init.OwnAddress2 = 0;
  hi2c2.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c2.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C2_Init 2 */

  /* USER CODE END I2C2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 71;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 65535;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pins : PA5 PA6 PA7 */
  GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6|GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB12 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_12;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PB13 PB14 */
  GPIO_InitStruct.Pin = GPIO_PIN_13|GPIO_PIN_14;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

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

#ifdef  USE_FULL_ASSERT
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
