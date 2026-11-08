/**
 * @file app_irq.c
 * @brief 应用层中断回调分发。
 */

#include "app_irq.h"

#include "app_main.h"

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size) {
  if (huart == usartInstance.handle) {
    BSP_USART_HandleRxEvent(&usartInstance, size);
  }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
  if (huart == usartInstance.handle) {
    BSP_USART_HandleTxComplete(&usartInstance);
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
  if (huart == usartInstance.handle) {
    BSP_USART_HandleError(&usartInstance);
  }
}
