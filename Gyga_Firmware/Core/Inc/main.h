/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32l4xx_hal.h"

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

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ANA1_Pin GPIO_PIN_0
#define ANA1_GPIO_Port GPIOC
#define ANB1_Pin GPIO_PIN_1
#define ANB1_GPIO_Port GPIOC
#define ANA2_Pin GPIO_PIN_2
#define ANA2_GPIO_Port GPIOC
#define ANB2_Pin GPIO_PIN_3
#define ANB2_GPIO_Port GPIOC
#define ANA3_Pin GPIO_PIN_0
#define ANA3_GPIO_Port GPIOA
#define ANB3_Pin GPIO_PIN_1
#define ANB3_GPIO_Port GPIOA
#define ANA4_Pin GPIO_PIN_2
#define ANA4_GPIO_Port GPIOA
#define ANB4_Pin GPIO_PIN_3
#define ANB4_GPIO_Port GPIOA
#define MCS_Pin GPIO_PIN_4
#define MCS_GPIO_Port GPIOA
#define MSCK_Pin GPIO_PIN_5
#define MSCK_GPIO_Port GPIOA
#define MSO_Pin GPIO_PIN_6
#define MSO_GPIO_Port GPIOA
#define MSI_Pin GPIO_PIN_7
#define MSI_GPIO_Port GPIOA
#define T_RS485_Pin GPIO_PIN_4
#define T_RS485_GPIO_Port GPIOC
#define R_RS485_Pin GPIO_PIN_5
#define R_RS485_GPIO_Port GPIOC
#define ANA5_Pin GPIO_PIN_0
#define ANA5_GPIO_Port GPIOB
#define ANB5_Pin GPIO_PIN_1
#define ANB5_GPIO_Port GPIOB
#define RX_DIS_Pin GPIO_PIN_10
#define RX_DIS_GPIO_Port GPIOB
#define TX_DIS_Pin GPIO_PIN_11
#define TX_DIS_GPIO_Port GPIOB
#define TXDBG_Pin GPIO_PIN_9
#define TXDBG_GPIO_Port GPIOA
#define RXDBG_Pin GPIO_PIN_10
#define RXDBG_GPIO_Port GPIOA
#define SCL_Pin GPIO_PIN_6
#define SCL_GPIO_Port GPIOB
#define SDA_Pin GPIO_PIN_7
#define SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
