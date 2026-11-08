/**
 * @file inv_mpu_port.c
 * @brief InvenSense 驱动的平台接口实现。
 */

#include "inv_mpu_port.h"

#include <stddef.h>

#define INV_MPU_I2C_TIMEOUT_MS 100U

static I2C_Hardware_t sInvMpuI2c;

/** @brief 初始化 InvenSense 平台接口。 */
int InvMpuPort_Init(I2C_Bus_t *i2cBus, uint8_t address7Bit) {
  if ((I2C_Hardware_Attach(&sInvMpuI2c, i2cBus, address7Bit) != HAL_OK) ||
      (I2C_Hardware_IsReady(&sInvMpuI2c, 3U, INV_MPU_I2C_TIMEOUT_MS) != HAL_OK)) {
    return -1;
  }

  return 0;
}

/** @brief 写入器件寄存器。 */
int InvMpuPort_I2cWrite(uint8_t slaveAddress, uint8_t registerAddress, uint8_t length, const uint8_t *data) {
  (void)slaveAddress;

  if ((data == NULL) || (length == 0U)) {
    return -1;
  }

  if (I2C_Hardware_MemWrite(&sInvMpuI2c, registerAddress, I2C_MEMADD_SIZE_8BIT, data, length, INV_MPU_I2C_TIMEOUT_MS) ==
      HAL_OK) {
    return 0;
  }

  return -1;
}

/** @brief 读取器件寄存器。 */
int InvMpuPort_I2cRead(uint8_t slaveAddress, uint8_t registerAddress, uint8_t length, uint8_t *data) {
  (void)slaveAddress;

  if ((data == NULL) || (length == 0U)) {
    return -1;
  }

  if (I2C_Hardware_MemRead(&sInvMpuI2c, registerAddress, I2C_MEMADD_SIZE_8BIT, data, length, INV_MPU_I2C_TIMEOUT_MS) ==
      HAL_OK) {
    return 0;
  }
  return -1;
}

/** @brief 毫秒延时。 */
void InvMpuPort_DelayMs(unsigned long milliseconds) { HAL_Delay(milliseconds); }

/** @brief 获取系统毫秒时间。 */
int InvMpuPort_GetMs(unsigned long *milliseconds) {
  if (milliseconds == NULL) {
    return -1;
  }

  *milliseconds = HAL_GetTick();
  return 0;
}
