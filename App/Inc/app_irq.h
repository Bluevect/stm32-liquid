/**
 * @file app_irq.h
 * @brief 应用层中断回调接口。
 */

#ifndef STM32_OLED_MPU6050_APP_IRQ_H
#define STM32_OLED_MPU6050_APP_IRQ_H

#include <stdint.h>

#include "stm32f1xx_hal.h"

/**
 * @brief 处理串口接收事件。
 * @param[in,out] huart UART 句柄。
 * @param[in] size 接收长度。
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size);

/**
 * @brief 处理串口发送完成事件。
 * @param[in,out] huart UART 句柄。
 */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);

/**
 * @brief 处理串口错误事件。
 * @param[in,out] huart UART 句柄。
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);

#endif // STM32_OLED_MPU6050_APP_IRQ_H
