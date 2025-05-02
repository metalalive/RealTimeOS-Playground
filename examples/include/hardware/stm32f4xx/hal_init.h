/* USER CODE BEGIN Header */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __HAL_INIT_H
#define __HAL_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

// TODO, refactor to support more hardware devices
#include "stm32f4xx_hal.h"

void Error_Handler(void);

#ifdef __cplusplus
}
#endif
#endif /* __HAL_INIT_H */
