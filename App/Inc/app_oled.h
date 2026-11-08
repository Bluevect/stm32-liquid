/**
 * @file app_oled.h
 * @brief OLED 应用显示接口（裸机刷新，不依赖 RTOS）。
 */

#ifndef STM32_OLED_MPU6050_APP_OLED_H
#define STM32_OLED_MPU6050_APP_OLED_H

#include <stdbool.h>

#include "bsp_oled.h"

/**
 * @brief 初始化 OLED 并显示启动画面。
 * @param[out] display OLED 实例。
 * @param[in] i2cBus 所属 I2C 总线。
 * @return 初始化成功返回 true，否则返回 false。
 */
bool AppOled_Init(BSP_OLED_t *display, I2C_Bus_t *i2cBus);

/**
 * @brief 刷新一帧：液体 + HUD 文本，然后推送到屏幕。
 * @param[in] nowMs 当前毫秒时刻（保留给后续动画使用）。
 */
void AppOled_Refresh(uint32_t nowMs);

/**
 * @brief 查询 OLED 是否可用。
 * @return 可用返回 true。
 */
bool AppOled_IsReady(void);

#endif // STM32_OLED_MPU6050_APP_OLED_H
