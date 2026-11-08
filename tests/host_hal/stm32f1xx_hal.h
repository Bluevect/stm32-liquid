/**
 * @file stm32f1xx_hal.h
 * @brief 主机端（PC）编译 OLED BSP 时使用的最小 HAL 替身。
 *
 * 只提供 Drivers/BSP/Src/bsp_oled.c 实际用到的类型和函数，用于在 PC 上复算
 * 固件显存内容，与实机 SWD 读回的 1024 字节逐字节比对。
 */

#ifndef HOST_STM32F1XX_HAL_H
#define HOST_STM32F1XX_HAL_H

#include <stdint.h>

typedef enum {
  HAL_OK = 0x00U,
  HAL_ERROR = 0x01U,
  HAL_BUSY = 0x02U,
  HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

typedef struct {
  uint32_t dummy;
} I2C_HandleTypeDef;

void HAL_Delay(uint32_t milliseconds);

#endif // HOST_STM32F1XX_HAL_H
