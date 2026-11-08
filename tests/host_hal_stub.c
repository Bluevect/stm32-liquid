/**
 * @file host_hal_stub.c
 * @brief 主机端 I2C 平台层替身（与 bsp_oled.c 一起在 PC 上编译）。
 */

#include "i2c_hardware.h"

#include <stddef.h>
#include <stdio.h>

/** 主机端记录的总线写次数，便于调试。 */
unsigned g_host_i2c_writes;

void HAL_Delay(uint32_t milliseconds) { (void)milliseconds; }

HAL_StatusTypeDef I2C_Bus_SetSpeed(I2C_Bus_t *bus, uint32_t clockHz) {
  (void)bus;
  (void)clockHz;
  return HAL_OK;
}

HAL_StatusTypeDef I2C_Hardware_Attach(I2C_Hardware_t *device, I2C_Bus_t *bus, uint8_t address7Bit) {
  (void)bus;
  (void)address7Bit;
  if (device == NULL) {
    return HAL_ERROR;
  }
  device->bus = bus;
  device->address = 0x78U;
  return HAL_OK;
}

HAL_StatusTypeDef I2C_Hardware_IsReady(I2C_Hardware_t *device, uint32_t trials, uint32_t timeout) {
  (void)device;
  (void)trials;
  (void)timeout;
  return HAL_OK;
}

HAL_StatusTypeDef I2C_Hardware_Write(I2C_Hardware_t *device, const uint8_t *data, uint16_t size, uint32_t timeout) {
  (void)device;
  (void)data;
  (void)size;
  (void)timeout;
  ++g_host_i2c_writes;
  return HAL_OK;
}

HAL_StatusTypeDef I2C_Hardware_Read(I2C_Hardware_t *device, uint8_t *data, uint16_t size, uint32_t timeout) {
  (void)device;
  if (data != NULL) {
    for (uint16_t i = 0U; i < size; ++i) {
      data[i] = 0U;
    }
  }
  (void)timeout;
  return HAL_OK;
}

HAL_StatusTypeDef I2C_Hardware_MemWrite(I2C_Hardware_t *device, uint16_t memoryAddress, uint16_t memoryAddressSize,
                                        const uint8_t *data, uint16_t size, uint32_t timeout) {
  (void)device;
  (void)memoryAddress;
  (void)memoryAddressSize;
  (void)data;
  (void)size;
  (void)timeout;
  return HAL_OK;
}

HAL_StatusTypeDef I2C_Hardware_MemRead(I2C_Hardware_t *device, uint16_t memoryAddress, uint16_t memoryAddressSize,
                                       uint8_t *data, uint16_t size, uint32_t timeout) {
  (void)device;
  (void)memoryAddress;
  (void)memoryAddressSize;
  (void)timeout;
  if (data != NULL) {
    for (uint16_t i = 0U; i < size; ++i) {
      data[i] = 0U;
    }
  }
  return HAL_OK;
}
