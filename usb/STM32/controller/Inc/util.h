#pragma once

//
// Helper macro
//

#define OFFSET_OF(T, F) ((size_t)&((T*)(0))->F)
#define SIZE_WITH(T, F) (OFFSET_OF(T, F) + sizeof(((T*)(0))->F))

#define ARR_SZ(a) (sizeof(a)/sizeof(a[0]))
#define STRZ_LEN(s) (ARR_SZ(s)-1)

#define xstr(a) _str(a)
#define _str(a) #a

#define READ_PIN(PIN) (HAL_GPIO_ReadPin(PIN##_GPIO_Port, PIN##_Pin) == GPIO_PIN_SET)
#define WRITE_PIN(PIN, val) HAL_GPIO_WritePin(PIN##_GPIO_Port, PIN##_Pin, (val) ? GPIO_PIN_SET : GPIO_PIN_RESET)
#define SETUP_PIN(PIN, MODE, PULL) do { \
		GPIO_InitTypeDef GPIO_InitStruct = { \
			.Pin = PIN##_Pin, .Mode = MODE, .Pull = PULL, .Speed = GPIO_SPEED_FREQ_LOW }; \
		HAL_GPIO_Init(PIN##_GPIO_Port, &GPIO_InitStruct); \
	} while (0)
