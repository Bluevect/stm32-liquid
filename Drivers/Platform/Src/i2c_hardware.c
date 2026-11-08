/**
 * @file i2c_hardware.c
 * @brief I2C 总线与设备访问实现。
 */

#include "i2c_hardware.h"

#include <stddef.h>

/** @brief 初始化 I2C 总线。 */
bool I2C_Bus_Init(I2C_Bus_t *bus, I2C_HandleTypeDef *handle) {
  if (!bus || !handle) {
    return false;
  }

  bus->handle = handle;
  return true;
}

/**
 * @brief 设置总线时钟频率（重新初始化 HAL 句柄）。
 * @param[in,out] bus I2C 总线。
 * @param[in] clockHz 目标时钟，单位 Hz。
 * @return HAL 执行状态。
 */
HAL_StatusTypeDef I2C_Bus_SetSpeed(I2C_Bus_t *bus, uint32_t clockHz) {
  if ((bus == NULL) || (bus->handle == NULL) || (clockHz == 0U)) {
    return HAL_ERROR;
  }

  if (bus->handle->Init.ClockSpeed == clockHz) {
    return HAL_OK;
  }

  bus->handle->Init.ClockSpeed = clockHz;
  if (HAL_I2C_DeInit(bus->handle) != HAL_OK) {
    return HAL_ERROR;
  }
  return HAL_I2C_Init(bus->handle);
}

/** @brief 将设备绑定到 I2C 总线。 */
HAL_StatusTypeDef I2C_Hardware_Attach(I2C_Hardware_t *device, I2C_Bus_t *bus, uint8_t address7Bit) {
  if (!device || !bus || !bus->handle) {
    return HAL_ERROR;
  }

  device->bus = bus;
  device->address = (uint16_t)((address7Bit & 0x7FU) << 1U);
  return HAL_OK;
}

/** @brief 检查设备是否响应。 */
HAL_StatusTypeDef I2C_Hardware_IsReady(I2C_Hardware_t *device, uint32_t trials, uint32_t timeout) {
  HAL_StatusTypeDef status;

  if (!device || !device->bus || !device->bus->handle) {
    return HAL_ERROR;
  }

  status = HAL_I2C_IsDeviceReady(device->bus->handle, device->address, trials, timeout);
  return status;
}

/** @brief 向设备发送数据。 */
HAL_StatusTypeDef I2C_Hardware_Write(I2C_Hardware_t *device, const uint8_t *data, uint16_t size, uint32_t timeout) {
  HAL_StatusTypeDef status;

  if (!device || !device->bus || !device->bus->handle || !data || (size == 0U)) {
    return HAL_ERROR;
  }

  status = HAL_I2C_Master_Transmit(device->bus->handle, device->address, (uint8_t *)data, size, timeout);
  return status;
}

/** @brief 从设备读取数据。 */
HAL_StatusTypeDef I2C_Hardware_Read(I2C_Hardware_t *device, uint8_t *data, uint16_t size, uint32_t timeout) {
  HAL_StatusTypeDef status;

  if (!device || !device->bus || !device->bus->handle || !data || (size == 0U)) {
    return HAL_ERROR;
  }

  status = HAL_I2C_Master_Receive(device->bus->handle, device->address, data, size, timeout);
  return status;
}

/** @brief 写入设备寄存器。 */
HAL_StatusTypeDef I2C_Hardware_MemWrite(I2C_Hardware_t *device, uint16_t memoryAddress, uint16_t memoryAddressSize,
                                        const uint8_t *data, uint16_t size, uint32_t timeout) {
  HAL_StatusTypeDef status;

  if (!device || !device->bus || !device->bus->handle || !data || (size == 0U)) {
    return HAL_ERROR;
  }

  status = HAL_I2C_Mem_Write(device->bus->handle, device->address, memoryAddress, memoryAddressSize, (uint8_t *)data,
                             size, timeout);
  return status;
}

/** @brief 读取设备寄存器。 */
HAL_StatusTypeDef I2C_Hardware_MemRead(I2C_Hardware_t *device, uint16_t memoryAddress, uint16_t memoryAddressSize,
                                       uint8_t *data, uint16_t size, uint32_t timeout) {
  HAL_StatusTypeDef status;

  if (!device || !device->bus || !device->bus->handle || !data || (size == 0U)) {
    return HAL_ERROR;
  }

  status =
      HAL_I2C_Mem_Read(device->bus->handle, device->address, memoryAddress, memoryAddressSize, data, size, timeout);
  return status;
}
