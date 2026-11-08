/**
 * @file bsp_mpu6050.c
 * @brief MPU6050 寄存器访问实现。
 */

#include "bsp_mpu6050.h"

#include <stddef.h>

#include "bsp_mpu6050_reg.h"

#define MPU6050_I2C_TIMEOUT 100U

/**
 * @brief 将输出数据清零。
 * @param[out] accX X 轴加速度，可为 NULL。
 * @param[out] accY Y 轴加速度，可为 NULL。
 * @param[out] accZ Z 轴加速度，可为 NULL。
 * @param[out] gyroX X 轴角速度，可为 NULL。
 * @param[out] gyroY Y 轴角速度，可为 NULL。
 * @param[out] gyroZ Z 轴角速度，可为 NULL。
 */
static void MPU6050_ClearData(int16_t *accX, int16_t *accY, int16_t *accZ, int16_t *gyroX, int16_t *gyroY,
                              int16_t *gyroZ) {
  if (accX != NULL) {
    *accX = 0;
  }
  if (accY != NULL) {
    *accY = 0;
  }
  if (accZ != NULL) {
    *accZ = 0;
  }
  if (gyroX != NULL) {
    *gyroX = 0;
  }
  if (gyroY != NULL) {
    *gyroY = 0;
  }
  if (gyroZ != NULL) {
    *gyroZ = 0;
  }
}

/**
 * @brief 写入单个寄存器。
 * @param[in,out] device MPU6050 实例。
 * @param[in] reg 寄存器地址。
 * @param[in] value 写入值。
 * @return 成功返回 true，否则返回 false。
 */
bool MPU6050_WriteReg(MPU6050_t *device, uint8_t reg, uint8_t value) {
  if (device == NULL) {
    return false;
  }

  return I2C_Hardware_MemWrite(&device->i2c, reg, I2C_MEMADD_SIZE_8BIT, &value, 1U, MPU6050_I2C_TIMEOUT) == HAL_OK;
}

/**
 * @brief 读取单个寄存器。
 * @param[in,out] device MPU6050 实例。
 * @param[in] reg 寄存器地址。
 * @param[out] value 读取值。
 * @return 成功返回 true，否则返回 false。
 */
bool MPU6050_ReadReg(MPU6050_t *device, uint8_t reg, uint8_t *value) {
  if ((device == NULL) || (value == NULL)) {
    return false;
  }

  return I2C_Hardware_MemRead(&device->i2c, reg, I2C_MEMADD_SIZE_8BIT, value, 1U, MPU6050_I2C_TIMEOUT) == HAL_OK;
}

/**
 * @brief 初始化普通寄存器模式。
 * @param[out] device MPU6050 实例。
 * @param[in] i2cBus 所属 I2C 总线。
 * @return 成功返回 true，否则返回 false。
 */
bool MPU6050_Init(MPU6050_t *device, I2C_Bus_t *i2cBus) {
  static const uint8_t configuration[][2] = {
      {MPU6050_PWR_MGMT_1, 0x01U}, {MPU6050_PWR_MGMT_2, 0x00U},  {MPU6050_SMPLRT_DIV, 0x09U},
      {MPU6050_CONFIG, 0x06U},     {MPU6050_GYRO_CONFIG, 0x18U}, {MPU6050_ACCEL_CONFIG, 0x18U},
  };

  if ((device == NULL) || (i2cBus == NULL)) {
    return false;
  }

  if ((I2C_Hardware_Attach(&device->i2c, i2cBus, 0x68U) != HAL_OK) ||
      (I2C_Hardware_IsReady(&device->i2c, 3U, MPU6050_I2C_TIMEOUT) != HAL_OK)) {
    return false;
  }

  for (uint32_t i = 0U; i < (sizeof(configuration) / sizeof(configuration[0])); ++i) {
    if (!MPU6050_WriteReg(device, configuration[i][0], configuration[i][1])) {
      return false;
    }
  }

  return true;
}

/**
 * @brief 读取设备 ID。
 * @param[in,out] device MPU6050 实例。
 * @param[out] id 设备 ID。
 * @return 成功返回 true，否则返回 false。
 */
bool MPU6050_GetID(MPU6050_t *device, uint8_t *id) { return MPU6050_ReadReg(device, MPU6050_WHO_AM_I, id); }

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
                     int16_t *gyroZ) {
  uint8_t data[14];

  if ((device == NULL) || (I2C_Hardware_MemRead(&device->i2c, MPU6050_ACCEL_XOUT_H, I2C_MEMADD_SIZE_8BIT, data,
                                                sizeof(data), MPU6050_I2C_TIMEOUT) != HAL_OK)) {
    MPU6050_ClearData(accX, accY, accZ, gyroX, gyroY, gyroZ);
    return false;
  }

  if (accX != NULL) {
    *accX = (int16_t)(((uint16_t)data[0] << 8U) | data[1]);
  }
  if (accY != NULL) {
    *accY = (int16_t)(((uint16_t)data[2] << 8U) | data[3]);
  }
  if (accZ != NULL) {
    *accZ = (int16_t)(((uint16_t)data[4] << 8U) | data[5]);
  }
  if (gyroX != NULL) {
    *gyroX = (int16_t)(((uint16_t)data[8] << 8U) | data[9]);
  }
  if (gyroY != NULL) {
    *gyroY = (int16_t)(((uint16_t)data[10] << 8U) | data[11]);
  }
  if (gyroZ != NULL) {
    *gyroZ = (int16_t)(((uint16_t)data[12] << 8U) | data[13]);
  }

  return true;
}
