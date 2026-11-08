/**
 * @file inv_mpu_port.h
 * @brief InvenSense 驱动的平台接口。
 */

#ifndef STM32_OLED_MPU6050_INV_MPU_PORT_H
#define STM32_OLED_MPU6050_INV_MPU_PORT_H

#include <stdint.h>

#include "i2c_hardware.h"

/**
 * @brief 初始化 InvenSense 平台接口。
 * @param[in] i2cBus I2C 总线。
 * @param[in] address7Bit 器件 7 位地址。
 * @return 成功返回 0，否则返回 -1。
 */
int InvMpuPort_Init(I2C_Bus_t *i2cBus, uint8_t address7Bit);

/**
 * @brief 写入器件寄存器。
 * @param[in] slaveAddress 器件地址。
 * @param[in] registerAddress 寄存器地址。
 * @param[in] length 数据长度。
 * @param[in] data 待写入数据。
 * @return 成功返回 0，否则返回 -1。
 */
int InvMpuPort_I2cWrite(uint8_t slaveAddress, uint8_t registerAddress, uint8_t length, const uint8_t *data);

/**
 * @brief 读取器件寄存器。
 * @param[in] slaveAddress 器件地址。
 * @param[in] registerAddress 寄存器地址。
 * @param[in] length 数据长度。
 * @param[out] data 接收缓冲。
 * @return 成功返回 0，否则返回 -1。
 */
int InvMpuPort_I2cRead(uint8_t slaveAddress, uint8_t registerAddress, uint8_t length, uint8_t *data);

/**
 * @brief 毫秒延时。
 * @param[in] milliseconds 延时时间。
 */
void InvMpuPort_DelayMs(unsigned long milliseconds);

/**
 * @brief 获取系统毫秒时间。
 * @param[out] milliseconds 当前毫秒数。
 * @return 成功返回 0，否则返回 -1。
 */
int InvMpuPort_GetMs(unsigned long *milliseconds);

#endif // STM32_OLED_MPU6050_INV_MPU_PORT_H
