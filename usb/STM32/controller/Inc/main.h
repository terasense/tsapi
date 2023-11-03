/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define REV0_Pin GPIO_PIN_13
#define REV0_GPIO_Port GPIOC
#define REV1_Pin GPIO_PIN_14
#define REV1_GPIO_Port GPIOC
#define REV2_Pin GPIO_PIN_15
#define REV2_GPIO_Port GPIOC
#define CFG_CHAIN0_Pin GPIO_PIN_0
#define CFG_CHAIN0_GPIO_Port GPIOC
#define CFG_CHAIN1_Pin GPIO_PIN_1
#define CFG_CHAIN1_GPIO_Port GPIOC
#define CFG_CHAIN2_Pin GPIO_PIN_2
#define CFG_CHAIN2_GPIO_Port GPIOC
#define CFG_WIDE_Pin GPIO_PIN_3
#define CFG_WIDE_GPIO_Port GPIOC
#define ADS_nFSYNC_Pin GPIO_PIN_2
#define ADS_nFSYNC_GPIO_Port GPIOA
#define ADS_RST_Pin GPIO_PIN_3
#define ADS_RST_GPIO_Port GPIOA
#define ADS_CONVST_Pin GPIO_PIN_6
#define ADS_CONVST_GPIO_Port GPIOA
#define FX_nHS_Pin GPIO_PIN_4
#define FX_nHS_GPIO_Port GPIOC
#define FX_nAFULL_Pin GPIO_PIN_5
#define FX_nAFULL_GPIO_Port GPIOC
#define FX_nFULL_Pin GPIO_PIN_0
#define FX_nFULL_GPIO_Port GPIOB
#define FX_nRST_Pin GPIO_PIN_1
#define FX_nRST_GPIO_Port GPIOB
#define FX_nPKTEND_Pin GPIO_PIN_2
#define FX_nPKTEND_GPIO_Port GPIOB
#define CFG_MOD0_Pin GPIO_PIN_10
#define CFG_MOD0_GPIO_Port GPIOB
#define CFG_MOD1_Pin GPIO_PIN_11
#define CFG_MOD1_GPIO_Port GPIOB
#define EPM_nCS_Pin GPIO_PIN_12
#define EPM_nCS_GPIO_Port GPIOB
#define DISPL_CS_Pin GPIO_PIN_6
#define DISPL_CS_GPIO_Port GPIOC
#define DISPL_DC_Pin GPIO_PIN_7
#define DISPL_DC_GPIO_Port GPIOC
#define DISPL_RST_Pin GPIO_PIN_8
#define DISPL_RST_GPIO_Port GPIOC
#define PSYNC_Pin GPIO_PIN_9
#define PSYNC_GPIO_Port GPIOA
#define PRESET_Pin GPIO_PIN_10
#define PRESET_GPIO_Port GPIOA
#define GPIO1_Pin GPIO_PIN_15
#define GPIO1_GPIO_Port GPIOA
#define GPIO2_Pin GPIO_PIN_10
#define GPIO2_GPIO_Port GPIOC
#define GPIO3_Pin GPIO_PIN_11
#define GPIO3_GPIO_Port GPIOC
#define GPIO4_Pin GPIO_PIN_12
#define GPIO4_GPIO_Port GPIOC
#define GPIO5_Pin GPIO_PIN_2
#define GPIO5_GPIO_Port GPIOD
#define GPO6_Pin GPIO_PIN_3
#define GPO6_GPIO_Port GPIOB
#define LED_GREEN_Pin GPIO_PIN_4
#define LED_GREEN_GPIO_Port GPIOB
#define LED_RED_Pin GPIO_PIN_5
#define LED_RED_GPIO_Port GPIOB
#define FSYNC2_Pin GPIO_PIN_8
#define FSYNC2_GPIO_Port GPIOB
#define CFG_MOD2_Pin GPIO_PIN_9
#define CFG_MOD2_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */
extern SPI_HandleTypeDef hspi1;
extern I2C_HandleTypeDef hi2c1;
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
