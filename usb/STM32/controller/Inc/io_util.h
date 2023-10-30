#pragma once

#include "stm32f4xx_hal.h"
#include <stdbool.h>

static inline bool ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
	return (GPIOx->IDR & GPIO_Pin) != 0;
}

static inline void WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, bool PinState)
{
	if (PinState)
		GPIOx->BSRR = GPIO_Pin;
	else
		GPIOx->BSRR = (uint32_t)GPIO_Pin << 16U;
}

#define READ_PIN(PIN) ReadPin(PIN##_GPIO_Port, PIN##_Pin)
#define WRITE_PIN(PIN, val) WritePin(PIN##_GPIO_Port, PIN##_Pin, (val) != 0)
#define SETUP_PIN(PIN, MODE, PULL) do { \
		GPIO_InitTypeDef GPIO_InitStruct = { \
			.Pin = PIN##_Pin, .Mode = MODE, .Pull = PULL, .Speed = GPIO_SPEED_FREQ_LOW }; \
		HAL_GPIO_Init(PIN##_GPIO_Port, &GPIO_InitStruct); \
	} while (0)
