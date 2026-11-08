/**
 * @file bsp_led.h
 * @brief 板载 LED 的 BSP 接口。
 *
 * 结构沿用 port/bsp_led.c：引脚由 CubeMX 在 main.h 里生成
 * （LED1_GPIO_Port / LED1_Pin），GPIO 初始化由 MX_GPIO_Init() 完成，
 * 本模块只负责使用。
 */

#ifndef STM32_OLED_MPU6050_BSP_LED_H
#define STM32_OLED_MPU6050_BSP_LED_H

/** @brief 翻转板载 LED1 的电平。 */
void Led1_Toggle(void);

#endif // STM32_OLED_MPU6050_BSP_LED_H
