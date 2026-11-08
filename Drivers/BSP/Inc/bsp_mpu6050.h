/**
 * @file bsp_mpu6050.h
 * @brief MPU6050 与 DMP 接口。
 */

#ifndef STM32_OLED_MPU6050_BSP_MPU6050_H
#define STM32_OLED_MPU6050_BSP_MPU6050_H

#include <stdbool.h>

#include "i2c_hardware.h"

/** @brief DMP 读取状态。 */
typedef enum {
  MPU6050_DMP_NO_DATA = 0, /**< 暂无数据。 */
  MPU6050_DMP_DATA_READY,  /**< 数据有效。 */
  MPU6050_DMP_ERROR        /**< 读取失败。 */
} MPU6050_DmpStatus_t;

/** @brief DMP 姿态数据。 */
typedef struct {
  int32_t quaternionQ30[4]; /**< Q30 四元数。 */
  int32_t rollDegX100;      /**< 横滚角，单位 0.01 度。 */
  int32_t pitchDegX100;     /**< 俯仰角，单位 0.01 度。 */
  int32_t yawDegX100;       /**< 航向角，单位 0.01 度。 */
  float gravityX;           /**< 板体系重力 x 分量（单位向量）。 */
  float gravityY;           /**< 板体系重力 y 分量（单位向量）。 */
  float gravityZ;           /**< 板体系重力 z 分量（单位向量）。 */
  uint32_t timestampMs;     /**< 时间戳，单位毫秒。 */
  bool valid;               /**< 数据是否有效。 */
} MPU6050_DmpData_t;

/** @brief MPU6050 实例。 */
typedef struct {
  I2C_Hardware_t i2c;
} MPU6050_t;

/**
 * @brief 初始化普通寄存器模式。
 * @param[out] device MPU6050 实例。
 * @param[in] i2cBus 所属 I2C 总线。
 * @return 成功返回 true，否则返回 false。
 */
bool MPU6050_Init(MPU6050_t *device, I2C_Bus_t *i2cBus);

/**
 * @brief 写入单个寄存器。
 * @param[in,out] device MPU6050 实例。
 * @param[in] reg 寄存器地址。
 * @param[in] value 写入值。
 * @return 成功返回 true，否则返回 false。
 */
bool MPU6050_WriteReg(MPU6050_t *device, uint8_t reg, uint8_t value);

/**
 * @brief 读取单个寄存器。
 * @param[in,out] device MPU6050 实例。
 * @param[in] reg 寄存器地址。
 * @param[out] value 读取值。
 * @return 成功返回 true，否则返回 false。
 */
bool MPU6050_ReadReg(MPU6050_t *device, uint8_t reg, uint8_t *value);

/**
 * @brief 读取设备 ID。
 * @param[in,out] device MPU6050 实例。
 * @param[out] id 设备 ID。
 * @return 成功返回 true，否则返回 false。
 */
bool MPU6050_GetID(MPU6050_t *device, uint8_t *id);

/**
 * @brief 读取原始加速度与角速度。
 * @param[in,out] device MPU6050 实例。
 * @param[out] accX X 轴加速度，可为 NULL。
 * @param[out] accY Y 轴加速度，可为 NULL。
 * @param[out] accZ Z 轴加速度，可为 NULL。
 * @param[out] gyroX X 轴角速度，可为 NULL。
 * @param[out] gyroY Y 轴角速度，可为 NULL。
 * @param[out] gyroZ Z 轴角速度，可为 NULL。
 * @return 成功返回 true，否则返回 false。
 */
bool MPU6050_GetData(MPU6050_t *device, int16_t *accX, int16_t *accY, int16_t *accZ, int16_t *gyroX, int16_t *gyroY,
                     int16_t *gyroZ);

/**
 * @brief 初始化并启动 DMP。
 * @param[out] device MPU6050 实例。
 * @param[in] i2cBus 所属 I2C 总线。
 * @return 成功返回 true，否则返回 false。
 */
bool MPU6050_InitDmp(MPU6050_t *device, I2C_Bus_t *i2cBus);

/**
 * @brief 读取最新 DMP 姿态数据。
 * @param[in] device MPU6050 实例。
 * @param[out] data 姿态数据。
 * @return DMP 读取状态。
 */
MPU6050_DmpStatus_t MPU6050_ReadDmp(const MPU6050_t *device, MPU6050_DmpData_t *data);

#endif // STM32_OLED_MPU6050_BSP_MPU6050_H
