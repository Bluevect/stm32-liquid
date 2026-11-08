/**
 * @file app_main.h
 * @brief 应用层初始化接口与共享实例（裸机超级循环，不使用 FreeRTOS）。
 */

#ifndef STM32_OLED_MPU6050_APP_MAIN_H
#define STM32_OLED_MPU6050_APP_MAIN_H

#include <stdbool.h>

#include "bsp_mpu6050.h"
#include "bsp_oled.h"
#include "bsp_usart.h"

/** @brief 串口实例。 */
extern BSP_USART_t usartInstance;
/** @brief OLED 实例。 */
extern BSP_OLED_t oledInstance;
/** @brief MPU6050 实例。 */
extern MPU6050_t mpu6050Instance;

/**
 * @brief 初始化应用硬件与调度器。
 * @note  MX_GPIO_Init / MX_DMA_Init / MX_USART1_UART_Init / MX_I2C1_Init /
 *        MX_I2C2_Init 必须先被调用。
 * @return 基础外设（I2C、串口）初始化成功返回 true，否则返回 false。
 */
bool AppMain_Init(void);

/**
 * @brief 应用主循环的一次迭代，main() 中死循环调用。
 */
void AppMain_Loop(void);

#endif // STM32_OLED_MPU6050_APP_MAIN_H
