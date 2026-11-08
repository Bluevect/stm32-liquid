/**
 * @file bsp_usart.h
 * @brief 串口收发接口。
 */

#ifndef STM32_OLED_MPU6050_BSP_USART_H
#define STM32_OLED_MPU6050_BSP_USART_H

#include <stdbool.h>

#include "stm32f1xx_hal.h"

/** @brief 单帧接收缓冲大小。 */
#define BSP_USART_RX_BUFFER_SIZE 256U
/** @brief 发送环形缓冲大小。 */
#define BSP_USART_TX_BUFFER_SIZE 1024U

typedef struct BSP_USART_t BSP_USART_t;

/**
 * @brief 串口帧接收回调。
 * @param[in] usart 串口实例。
 * @param[in] data 接收数据，仅回调期间有效。
 * @param[in] size 数据长度。
 */
typedef void (*BSP_USART_RxCallback_t)(BSP_USART_t *usart, const uint8_t *data, uint16_t size);

/** @brief 串口实例。 */
struct BSP_USART_t {
  UART_HandleTypeDef *handle;                 /**< HAL 串口句柄。 */
  BSP_USART_RxCallback_t rxCallback;          /**< 帧接收回调。 */
  uint8_t rxBuffer[BSP_USART_RX_BUFFER_SIZE]; /**< 接收缓冲。 */
  uint8_t txBuffer[BSP_USART_TX_BUFFER_SIZE]; /**< 发送缓冲。 */
  uint16_t txReadIndex;                       /**< 发送读位置。 */
  uint16_t txWriteIndex;                      /**< 发送写位置。 */
  uint16_t txInFlightLength;                  /**< 当前发送长度。 */
  bool txActive;                              /**< 发送状态。 */
};

/**
 * @brief 初始化串口并启动接收。
 * @param[out] usart 串口实例。
 * @param[in] uart HAL 串口句柄。
 * @param[in] rxCallback 帧接收回调。
 * @return 成功返回 true，否则返回 false。
 */
bool BSP_USART_Init(BSP_USART_t *usart, UART_HandleTypeDef *uart, BSP_USART_RxCallback_t rxCallback);

/**
 * @brief 异步发送数据。
 * @param[in,out] usart 串口实例。
 * @param[in] data 待发送数据。
 * @param[in] size 数据长度。
 * @return 成功加入发送缓冲返回 true，否则返回 false。
 */
bool BSP_USART_Transmit(BSP_USART_t *usart, const uint8_t *data, uint16_t size);

/**
 * @brief 处理接收事件。
 * @param[in,out] usart 串口实例。
 * @param[in] size 接收数据长度。
 */
void BSP_USART_HandleRxEvent(BSP_USART_t *usart, uint16_t size);
/**
 * @brief 处理发送完成事件。
 * @param[in,out] usart 串口实例。
 */
void BSP_USART_HandleTxComplete(BSP_USART_t *usart);
/**
 * @brief 处理串口错误。
 * @param[in,out] usart 串口实例。
 */
void BSP_USART_HandleError(BSP_USART_t *usart);

#endif // STM32_OLED_MPU6050_BSP_USART_H
