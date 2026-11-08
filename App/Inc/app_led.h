/**
 * @file app_led.h
 * @brief 板载 LED 状态指示。
 *
 * LED 的 GPIO 由 CubeMX 在 MX_GPIO_Init() 里初始化，本模块只负责按状态闪烁，
 * 底层通过 Drivers/BSP/bsp_led.c 的 Led1_Toggle() 操作引脚。
 */

#ifndef STM32_OLED_MPU6050_APP_LED_H
#define STM32_OLED_MPU6050_APP_LED_H

#include <stdint.h>

/** @brief LED 指示状态。 */
typedef enum {
  APP_LED_STATE_OK = 0, /**< 正常运行，1 Hz 慢闪。 */
  APP_LED_STATE_ERROR   /**< 故障，约 8 Hz 快闪。 */
} AppLedState_t;

/**
 * @brief 初始化 LED 指示模块。
 * @param[in] nowMs 当前毫秒时刻。
 */
void AppLed_Init(uint32_t nowMs);

/**
 * @brief 设置指示状态。
 * @param[in] state 新的状态。
 */
void AppLed_SetState(AppLedState_t state);

/**
 * @brief 周期调用，按当前状态翻转 LED。
 * @param[in] nowMs 当前毫秒时刻。
 */
void AppLed_Process(uint32_t nowMs);

#endif // STM32_OLED_MPU6050_APP_LED_H
