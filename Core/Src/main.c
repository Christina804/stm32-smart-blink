/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : 声光感应智能LED灯项目
  ******************************************************************************、
	* 硬件引脚分配：
  * PA0  -> ADC1_IN0  		光敏电阻LDR（光照采集）
  * PA1  -> GPIO_INPUT 		按键KEY1：切换【自动声光模式】
  * PA2  -> GPIO_INPUT 		按键KEY2：切换【手动亮度模式】
  * PA3  -> GPIO_INPUT 		按键KEY3：手动模式【亮度升档】
  * PA4  -> GPIO_INPUT 		按键KEY4：手动模式【亮度降档】
  * PB0  -> GPIO_INPUT 		声音传感器输出
  * PB1  -> TIM3_CH4  		PWM输出，驱动LED（三极管）
  * PB3  -> GPIO_OUTPUT 	模式状态指示灯
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
#include "adc.h"
#include "tim.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t  mode_auto = 0;      	// 模式标志：0=手动模式，1=自动声光模式
uint16_t pwm_duty = 0;        // PWM比较值，范围0~999，控制LED亮度
uint16_t ldr_val = 0;         // 光敏ADC采样值，12位，范围0~4095
uint8_t  sound_flag = 0;      // 声音传感器实时电平：1有声音，0无声音
uint8_t  sound_trig = 0;      // 声音触发标记：1=已经触发亮灯，0=未触发
uint16_t light_timer = 0;    	// 亮灯倒计时计数器，单位：10ms
#define LDR_THRESHOLD  100   	// 光照阈值：ldr_val>=2000判定强光，否则弱

// 4档亮度表：0熄灭，300暗，600中，900最亮（适配TIM3 ARR=999）
uint16_t brightness_table[4] = {0, 300, 600, 900};
static uint8_t bright_level = 1;	// 当前亮度档位，默认1档=300
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
  
	HAL_Init();				//初始化HAL库

  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();		//初始化系统时钟

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();		//初始化外设：GPIO、ADC1、TIM3定时器
  MX_ADC1_Init();		
  MX_TIM3_Init();
  /* USER CODE BEGIN 2 */
  HAL_ADCEx_Calibration_Start(&hadc1);		// STM32F1 ADC上电校准，保障采样精度
  HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_4);  	//启动TIM3 CH4的PWM输出，PB1输出PWM驱动LED
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, pwm_duty); 	//初始PWM赋值
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);			//上电初始化 PB3（模式指示灯），默认熄灭
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
		/************************ 【模块1：按键扫描模块】 ************************/
		// KEY1 PA1：切换自动声光模式
    if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET)
    {
      HAL_Delay(20);  // 软件消抖20ms
      if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET)
      {
        mode_auto = 1;
        light_timer = 0;
        sound_trig = 0;
        pwm_duty = 0;
      }
      while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_1) == GPIO_PIN_RESET);  // 等待松手
    }
		
		// KEY2 PA2：切换手动亮度模式
    if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_RESET)
    {
      HAL_Delay(20);
      if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_RESET)
      {
        mode_auto = 0;
        pwm_duty = brightness_table[bright_level];
      }
      while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_RESET);
    }

    // 只有手动模式，才允许亮度加减按键生效
    if(mode_auto == 0)
    {
      // KEY3 PA3：亮度升档
      if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET)
			{
				HAL_Delay(20);
				if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET)
				{
					bright_level ++;
					if(bright_level >= 4) bright_level = 3; 	// 封顶最高档
					pwm_duty = brightness_table[bright_level];
				}
				while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_3) == GPIO_PIN_RESET);
			}
			
			// KEY4 PA4：亮度降档
			if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET)
			{
				HAL_Delay(20);
				if(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET)
				{
					bright_level --;
					if(bright_level > 3) bright_level = 0; 		// 最低到熄灭档
					pwm_duty = brightness_table[bright_level];
				}
				while(HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_4) == GPIO_PIN_RESET);
				}
			}
		
		/************************ 【模块2：光敏ADC采集模块 PA0】 ************************/
    HAL_ADC_Start(&hadc1);		// 启动ADC转换
    if(HAL_ADC_PollForConversion(&hadc1, 50) == HAL_OK)		// 等待转换完成，超时50ms
    {
      ldr_val = HAL_ADC_GetValue(&hadc1);		// 读取光照数值
    }
    HAL_ADC_Stop(&hadc1);		// 关闭ADC

		/************************ 【模块3：声音传感器采集模块 PB0】 ************************/
    sound_flag = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0);		// 获取声音传感器电平

		/************************ 【模块4：自动模式声光控制逻辑】 ************************/
    if(mode_auto == 1)
    {
      // 32位临时变量，防止乘法溢出，把0~4095映射到0~600亮度区间
     uint32_t temp_bright = (uint32_t)ldr_val * 600 / 4095;

      if(ldr_val >= LDR_THRESHOLD)
      {
        // 强光环境：LED直接常亮，声音不影响
				pwm_duty = (uint16_t)temp_bright;
        sound_trig = 0;
        light_timer = 0;
      }
      else
      {
        // 弱光环境：检测到声音才点亮，倒计时自动熄灭
        if(sound_flag == GPIO_PIN_SET)
        {
          sound_trig = 1;			// 标记：已经被声音触发，灯要打开
          light_timer = 200; 	// 200*10ms = 2秒点亮时长
        }

        if(sound_trig == 1)
        {
          pwm_duty = (uint16_t)temp_bright;		// 打开LED，亮度由当前光敏值决定
          light_timer--;
          if(light_timer == 0)
          {
            sound_trig = 0;
            pwm_duty = 0;
          }
        }
        else
        {
          pwm_duty = 0;
        }
      }
    }

    /************************ 【模块5：PWM输出模块 PB1 TIM3_CH4】 ************************/
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_4, pwm_duty);

    /************************ 【模块6：模式指示灯 PB3】 ************************/
    if(mode_auto == 1)
    {
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);   // 自动模式：PB3亮
    }
    else
    {
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET); // 手动模式：PB3灭
    }

    HAL_Delay(10);
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
