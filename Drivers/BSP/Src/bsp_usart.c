/**
 * @file bsp_usart.c
 * @brief 串口 DMA 收发实现。
 */

#include "bsp_usart.h"

/**
 * @brief 进入短临界区。
 * @return 进入前的 PRIMASK。
 */
static uint32_t USART_EnterCritical(void) {
  uint32_t primask = __get_PRIMASK();
  __disable_irq();
  return primask;
}

/**
 * @brief 退出短临界区。
 * @param[in] primask 进入临界区前的 PRIMASK。
 */
static void USART_ExitCritical(uint32_t primask) { __set_PRIMASK(primask); }

/**
 * @brief 获取发送缓冲已用长度。
 * @param[in] usart 串口实例。
 * @return 已用字节数。
 */
static uint16_t USART_GetTxUsed(const BSP_USART_t *usart) {
  if (usart->txWriteIndex >= usart->txReadIndex) {
    return (uint16_t)(usart->txWriteIndex - usart->txReadIndex);
  }

  return (uint16_t)(BSP_USART_TX_BUFFER_SIZE - usart->txReadIndex + usart->txWriteIndex);
}

/**
 * @brief 将数据加入发送缓冲。
 * @param[in,out] usart 串口实例。
 * @param[in] data 待发送数据。
 * @param[in] length 数据长度。
 * @return 空间足够返回 true，否则返回 false。
 */
static bool USART_Enqueue(BSP_USART_t *usart, const uint8_t *data, uint16_t length) {
  uint32_t primask = USART_EnterCritical();
  uint16_t freeSize = (uint16_t)(BSP_USART_TX_BUFFER_SIZE - 1U - USART_GetTxUsed(usart));

  if (length > freeSize) {
    USART_ExitCritical(primask);
    return false;
  }

  for (uint16_t i = 0U; i < length; i++) {
    usart->txBuffer[usart->txWriteIndex] = data[i];
    usart->txWriteIndex++;

    if (usart->txWriteIndex == BSP_USART_TX_BUFFER_SIZE) {
      usart->txWriteIndex = 0U;
    }
  }

  USART_ExitCritical(primask);
  return true;
}

/**
 * @brief 启动下一段 DMA 发送。
 * @param[in,out] usart 串口实例。
 */
static void USART_StartNextTransmit(BSP_USART_t *usart) {
  uint32_t primask = USART_EnterCritical();
  uint16_t chunkLength;

  if (usart->txActive) {
    USART_ExitCritical(primask);
    return;
  }

  if (usart->txWriteIndex >= usart->txReadIndex) {
    chunkLength = (uint16_t)(usart->txWriteIndex - usart->txReadIndex);
  } else {
    chunkLength = (uint16_t)(BSP_USART_TX_BUFFER_SIZE - usart->txReadIndex);
  }

  if (chunkLength == 0U) {
    USART_ExitCritical(primask);
    return;
  }

  usart->txInFlightLength = chunkLength;
  usart->txActive = true;
  if (HAL_UART_Transmit_DMA(usart->handle, &usart->txBuffer[usart->txReadIndex], chunkLength) != HAL_OK) {
    usart->txActive = false;
    usart->txInFlightLength = 0U;
  }
  USART_ExitCritical(primask);
}

/**
 * @brief 启动 DMA 接收。
 * @param[in,out] usart 串口实例。
 * @return HAL 执行状态。
 */
static HAL_StatusTypeDef USART_StartReceive(BSP_USART_t *usart) {
  return HAL_UARTEx_ReceiveToIdle_DMA(usart->handle, usart->rxBuffer, BSP_USART_RX_BUFFER_SIZE);
}

bool BSP_USART_Init(BSP_USART_t *usart, UART_HandleTypeDef *uart, BSP_USART_RxCallback_t rxCallback) {
  usart->handle = uart;
  usart->rxCallback = rxCallback;
  usart->txReadIndex = 0U;
  usart->txWriteIndex = 0U;
  usart->txInFlightLength = 0U;
  usart->txActive = false;

  return USART_StartReceive(usart) == HAL_OK;
}

bool BSP_USART_Transmit(BSP_USART_t *usart, const uint8_t *data, uint16_t size) {
  if (!USART_Enqueue(usart, data, size)) {
    return false;
  }

  USART_StartNextTransmit(usart);
  return true;
}

void BSP_USART_HandleRxEvent(BSP_USART_t *usart, uint16_t size) {
  if (HAL_UARTEx_GetRxEventType(usart->handle) == HAL_UART_RXEVENT_HT) {
    return;
  }

  if (size > BSP_USART_RX_BUFFER_SIZE) {
    size = (uint16_t)BSP_USART_RX_BUFFER_SIZE;
  }

  if (size > 0U) {
    usart->rxCallback(usart, usart->rxBuffer, size);
  }

  USART_StartReceive(usart);
}

void BSP_USART_HandleTxComplete(BSP_USART_t *usart) {
  uint32_t primask;

  primask = USART_EnterCritical();
  usart->txReadIndex = (uint16_t)(usart->txReadIndex + usart->txInFlightLength);
  if (usart->txReadIndex >= BSP_USART_TX_BUFFER_SIZE) {
    usart->txReadIndex = (uint16_t)(usart->txReadIndex - BSP_USART_TX_BUFFER_SIZE);
  }

  usart->txInFlightLength = 0U;
  usart->txActive = false;
  USART_ExitCritical(primask);

  USART_StartNextTransmit(usart);
}

void BSP_USART_HandleError(BSP_USART_t *usart) {
  uint32_t primask;

  primask = USART_EnterCritical();
  if (usart->txActive && (usart->handle->gState == HAL_UART_STATE_READY)) {
    usart->txReadIndex = (uint16_t)(usart->txReadIndex + usart->txInFlightLength);
    if (usart->txReadIndex >= BSP_USART_TX_BUFFER_SIZE) {
      usart->txReadIndex = (uint16_t)(usart->txReadIndex - BSP_USART_TX_BUFFER_SIZE);
    }
    usart->txInFlightLength = 0U;
    usart->txActive = false;
  }
  USART_ExitCritical(primask);

  USART_StartReceive(usart);
  USART_StartNextTransmit(usart);
}
