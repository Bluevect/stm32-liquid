/**
 * @file app_imu.h
 * @brief IMU 应用接口与样本定义（裸机轮询，不依赖 RTOS）。
 */

#ifndef STM32_OLED_MPU6050_APP_IMU_H
#define STM32_OLED_MPU6050_APP_IMU_H

#include <stdbool.h>
#include <stdint.h>

#include "bsp_mpu6050.h"

/** @brief IMU 数据源状态。 */
typedef enum {
  APP_IMU_STATUS_OK = 0, /**< DMP 数据正常。 */
  APP_IMU_STATUS_DEMO,   /**< 没有可用传感器，使用合成重力。 */
  APP_IMU_STATUS_ERROR   /**< 连续读取失败，保持最后一次有效值。 */
} AppImuStatus_t;

/** @brief IMU 姿态样本（屏幕坐标重力在第 5、6 项）。 */
typedef struct {
  uint32_t timestampMs; /**< 最近一次有效样本的时刻。 */
  int32_t rollDegX100;  /**< 横滚角，单位 0.01 度。 */
  int32_t pitchDegX100; /**< 俯仰角，单位 0.01 度。 */
  int32_t yawDegX100;   /**< 航向角，单位 0.01 度。 */
  float gravityX;       /**< 屏幕 x 方向重力，[-1, 1]，正值向右。 */
  float gravityY;       /**< 屏幕 y 方向重力，[-1, 1]，正值向下。 */
  bool valid;           /**< 数据是否有效。 */
} ImuSample_t;

/**
 * @brief 初始化 IMU 应用层。
 * @param[out] device MPU6050 实例。
 * @param[in] i2cBus 所属 I2C 总线。
 * @return 传感器可用返回 true；失败时自动进入 DEMO 模式并返回 false。
 */
bool AppImu_Init(MPU6050_t *device, I2C_Bus_t *i2cBus);

/**
 * @brief 轮询一次传感器并更新最新样本，不阻塞。
 * @param[in] nowMs 当前毫秒时刻。
 */
void AppImu_Poll(uint32_t nowMs);

/**
 * @brief 读取最新样本的只读指针。
 * @return 样本指针，永不为 NULL。
 */
const ImuSample_t *AppImu_Latest(void);

/**
 * @brief 读取 IMU 状态。
 * @return 当前状态。
 */
AppImuStatus_t AppImu_Status(void);

#endif // STM32_OLED_MPU6050_APP_IMU_H
