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
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "control.h"
#include "encoder.h"
#include "ins.h"
#include "knob.h"
#include "motor.h"
#include "oled.h"
#include "pid.h"
#include "vofa.h"
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

/* USER CODE BEGIN PV */
volatile uint8_t control_flag = 0U;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void FormatFixed1(char *buffer, size_t size, float value);    
static void OLED_ShowStatus(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void FormatFixed1(char *buffer, size_t size, float value)
{
    int scaled = (int)(value * 10.0f + (value >= 0.0f ? 0.5f : -0.5f));
    int whole = scaled / 10;
    int decimal = abs(scaled % 10);

    snprintf(buffer, size, "%d.%d", whole, decimal);
}

static void OLED_ShowStatus(void)
{
    ControlTelemetry_t telemetry;
    Control_GetTelemetry(&telemetry);
    ins_data_t *ins = ins_data();

    char target_angle[16];
    char actual_angle[16];
    char target_speed[16];
    char actual_speed[16];
    char error[16];
    char line[22];

    FormatFixed1(target_angle, sizeof(target_angle), telemetry.target_angle_deg);
    FormatFixed1(actual_angle, sizeof(actual_angle), telemetry.actual_angle_deg);
    FormatFixed1(target_speed, sizeof(target_speed), telemetry.target_speed_rpm);
    FormatFixed1(actual_speed, sizeof(actual_speed), telemetry.actual_speed_rpm);
    FormatFixed1(error, sizeof(error),
                 telemetry.target_angle_deg - telemetry.actual_angle_deg);

    OLED_NewFrame();
    snprintf(line, sizeof(line), "%s T:%.6s A:%.6s",
             telemetry.mode == CONTROL_MODE_ANGLE ? "ANG" : "SPD",
             target_angle, actual_angle);
    OLED_PrintASCIIString(0U, 0U, line, &afont12x6, OLED_COLOR_NORMAL);

    snprintf(line, sizeof(line), "ERR:%s", error);
    OLED_PrintASCIIString(0U, 16U, line, &afont12x6, OLED_COLOR_NORMAL);

    snprintf(line, sizeof(line), "SPD:%.6s/%.6s", target_speed, actual_speed);
    OLED_PrintASCIIString(0U, 32U, line, &afont12x6, OLED_COLOR_NORMAL);

    snprintf(line, sizeof(line), "PWM:%d K:%4u%s", (int)telemetry.pwm_output,
             (unsigned int)Knob_GetRaw(), ins->sensor_ok ? "" : " MPU!");
    OLED_PrintASCIIString(0U, 48U, line, &afont12x6, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
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
  MX_I2C1_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_I2C2_Init();
  /* USER CODE BEGIN 2 */

  Motor_Init();
  Encoder_Init();


  HAL_Delay(20U);
  OLED_Init();

  OLED_NewFrame();
  OLED_PrintASCIIString(0U, 24U, "HOLD ZERO ANGLE", &afont12x6, OLED_COLOR_NORMAL);
  OLED_ShowFrame();

  ins_init();
  Control_Init();
  Vofa_Init();

  HAL_TIM_Base_Start_IT(&htim3);

  uint32_t last_vofa_tick = HAL_GetTick();
  uint32_t last_oled_tick = last_vofa_tick;
  uint32_t last_control_tick = last_vofa_tick;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    Vofa_PollCommand();

    if (control_flag) {
      control_flag = 0U;
      uint32_t control_tick = HAL_GetTick();
      float control_dt = (float)(control_tick - last_control_tick) / 1000.0f;
      if (control_dt <= 0.0f) {
        control_dt = CONTROL_PERIOD_S;
      } else if (control_dt > 0.05f) {
        control_dt = 0.05f;
      }
      last_control_tick = control_tick;
      Control_Update(control_dt);
    }

    uint32_t now = HAL_GetTick();
    if ((now - last_vofa_tick) >= 50U) {
      last_vofa_tick = now;
      Vofa_SendTelemetry();
    }

    if ((now - last_oled_tick) >= 200U) {
      last_oled_tick = now;
      OLED_ShowStatus();
    }
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
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  static uint8_t control_divider = 0U;

  if (htim->Instance == TIM3)
  {
    control_divider++;
    if (control_divider >= CONTROL_PERIOD_MS)
    {
      control_divider = 0U;
      control_flag = 1U;
    }
  }
}
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

