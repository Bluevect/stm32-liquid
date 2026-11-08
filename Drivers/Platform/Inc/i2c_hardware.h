/**
 * @file i2c_hardware.h
 * @brief I2C 总线与设备访问接口。
 */

#ifndef STM32_OLED_MPU6050_I2C_HARDWARE_H
#define STM32_OLED_MPU6050_I2C_HARDWARE_H

#include <stdbool.h>
#include <stdint.h>

#include "stm32f1xx_hal.h"

/** @brief I2C 总线（裸机工程：没有 RTOS 互斥锁，总线由单一超级循环访问）。 */
typedef struct {
  I2C_HandleTypeDef *handle; /**< HAL I2C 句柄。 */
} I2C_Bus_t;

/** @brief I2C 设备。 */
typedef struct {
  I2C_Bus_t *bus;   /**< 所属总线。 */
  uint16_t address; /**< 8 位写地址。 */
} I2C_Hardware_t;

/**
 * @brief 初始化 I2C 总线。
 * @param[out] bus I2C 总线。
 * @param[in] handle HAL I2C 句柄。
 * @return 成功返回 true，否则返回 false。
 */
bool I2C_Bus_Init(I2C_Bus_t *bus, I2C_HandleTypeDef *handle);

/**
 * @brief 设置总线时钟频率（重新初始化 HAL 句柄）。
 * @param[in,out] bus I2C 总线。
 * @param[in] clockHz 目标时钟，单位 Hz，例如 400000。
 * @return HAL 执行状态。
 */
HAL_StatusTypeDef I2C_Bus_SetSpeed(I2C_Bus_t *bus, uint32_t clockHz);

/**
 * @brief 将设备绑定到 I2C 总线。
 * @param[out] device I2C 设备。
 * @param[in,out] bus I2C 总线。
 * @param[in] address7Bit 器件 7 位地址。
 * @return HAL 执行状态。
 */
HAL_StatusTypeDef I2C_Hardware_Attach(I2C_Hardware_t *device, I2C_Bus_t *bus, uint8_t address7Bit);

/**
 * @brief 检查设备是否响应。
 * @param[in,out] device I2C 设备。
 * @param[in] trials 重试次数。
 * @param[in] timeout 超时时间。
 * @return HAL 执行状态。
 */
HAL_StatusTypeDef I2C_Hardware_IsReady(I2C_Hardware_t *device, uint32_t trials, uint32_t timeout);

/**
 * @brief 向设备发送数据。
 * @param[in,out] device I2C 设备。
 * @param[in] data 待发送数据。
 * @param[in] size 数据长度。
 * @param[in] timeout 超时时间。
 * @return HAL 执行状态。
 */
HAL_StatusTypeDef I2C_Hardware_Write(I2C_Hardware_t *device, const uint8_t *data, uint16_t size, uint32_t timeout);

/**
 * @brief 从设备读取数据。
 * @param[in,out] device I2C 设备。
 * @param[out] data 接收缓冲。
 * @param[in] size 数据长度。
 * @param[in] timeout 超时时间。
 * @return HAL 执行状态。
 */
HAL_StatusTypeDef I2C_Hardware_Read(I2C_Hardware_t *device, uint8_t *data, uint16_t size, uint32_t timeout);

/**
 * @brief 写入设备寄存器。
 * @param[in,out] device I2C 设备。
 * @param[in] memoryAddress 寄存器地址。
 * @param[in] memoryAddressSize 寄存器地址宽度。
 * @param[in] data 待写入数据。
 * @param[in] size 数据长度。
 * @param[in] timeout 超时时间。
 * @return HAL 执行状态。
 */
HAL_StatusTypeDef I2C_Hardware_MemWrite(I2C_Hardware_t *device, uint16_t memoryAddress, uint16_t memoryAddressSize,
                                        const uint8_t *data, uint16_t size, uint32_t timeout);

/**
 * @brief 读取设备寄存器。
 * @param[in,out] device I2C 设备。
 * @param[in] memoryAddress 寄存器地址。
 * @param[in] memoryAddressSize 寄存器地址宽度。
 * @param[out] data 接收缓冲。
 * @param[in] size 数据长度。
 * @param[in] timeout 超时时间。
 * @return HAL 执行状态。
 */
HAL_StatusTypeDef I2C_Hardware_MemRead(I2C_Hardware_t *device, uint16_t memoryAddress, uint16_t memoryAddressSize,
                                       uint8_t *data, uint16_t size, uint32_t timeout);

#endif // STM32_OLED_MPU6050_I2C_HARDWARE_H
