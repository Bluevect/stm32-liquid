/**
 * @file app_utils.h
 * @brief 应用层通用工具（裸机周期调度，不依赖任何 RTOS）。
 */

#ifndef STM32_OLED_MPU6050_APP_UTILS_H
#define STM32_OLED_MPU6050_APP_UTILS_H

#include <stdbool.h>
#include <stdint.h>

/** @brief 一个周期任务的调度状态。 */
typedef struct {
  uint32_t nextMs;   /**< 下次到期时刻，单位毫秒。 */
  uint32_t periodMs; /**< 周期，单位毫秒。 */
} AppUtils_Periodic_t;

/**
 * @brief 初始化周期任务。
 * @param[out] timer 周期任务状态。
 * @param[in] nowMs 当前毫秒时刻。
 * @param[in] periodMs 周期，单位毫秒，必须大于 0。
 */
void AppUtils_PeriodicInit(AppUtils_Periodic_t *timer, uint32_t nowMs, uint32_t periodMs);

/**
 * @brief 判断周期任务是否到期，到期时自动推进到下一个周期。
 * @param[in,out] timer 周期任务状态。
 * @param[in] nowMs 当前毫秒时刻。
 * @return 到期返回 true，否则返回 false。
 */
bool AppUtils_PeriodicDue(AppUtils_Periodic_t *timer, uint32_t nowMs);

/**
 * @brief 计算两次毫秒时刻之差，结果对回绕安全。
 * @param[in] nowMs 当前时刻。
 * @param[in] thenMs 过去的时刻。
 * @return 经过的毫秒数。
 */
uint32_t AppUtils_ElapsedMs(uint32_t nowMs, uint32_t thenMs);

#endif // STM32_OLED_MPU6050_APP_UTILS_H
