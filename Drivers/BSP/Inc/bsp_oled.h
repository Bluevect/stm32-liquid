/**
 * @file bsp_oled.h
 * @brief OLED 显示接口。
 */

#ifndef STM32_OLED_MPU6050_BSP_OLED_H
#define STM32_OLED_MPU6050_BSP_OLED_H

#include <stdbool.h>
#include <stdint.h>

#include "i2c_hardware.h"

/** @brief 屏幕宽度。 */
#define BSP_OLED_WIDTH 128U
/** @brief 屏幕高度。 */
#define BSP_OLED_HEIGHT 64U
/** @brief 屏幕页数。 */
#define BSP_OLED_PAGES (BSP_OLED_HEIGHT / 8U)
/** @brief 显存大小。 */
#define BSP_OLED_BUFFER_SIZE (BSP_OLED_WIDTH * BSP_OLED_PAGES)
/** @brief 默认 I2C 地址。 */
#define BSP_OLED_DEFAULT_ADDRESS 0x3CU

/** @brief OLED 使用的 I2C 时钟频率。
 *  400 kHz 时整帧刷新约 23 ms；100 kHz 时约 92 ms（约 10 fps）。 */
#define BSP_OLED_I2C_SPEED_HZ 400000U

/** @brief OLED 实例。 */
typedef struct {
  I2C_Hardware_t i2c;
  uint8_t buffer[BSP_OLED_BUFFER_SIZE];
} BSP_OLED_t;

/**
 * @brief 初始化 OLED。
 * @param[out] display OLED 实例。
 * @param[in] i2cBus 所属 I2C 总线。
 * @return 成功返回 true，否则返回 false。
 */
bool BSP_OLED_Init(BSP_OLED_t *display, I2C_Bus_t *i2cBus);

/**
 * @brief 检查 OLED 是否响应。
 * @param[in,out] display OLED 实例。
 * @return 响应返回 true，否则返回 false。
 */
bool BSP_OLED_IsReady(BSP_OLED_t *display);

/**
 * @brief 将显存刷新到屏幕。
 * @param[in] display OLED 实例。
 * @return 成功返回 true，否则返回 false。
 */
bool BSP_OLED_Display(BSP_OLED_t *display);

/**
 * @brief 清空显存。
 * @param[in,out] display OLED 实例。
 */
void BSP_OLED_Clear(BSP_OLED_t *display);

/**
 * @brief 使用指定字节填充显存。
 * @param[in,out] display OLED 实例。
 * @param[in] pattern 填充值。
 */
void BSP_OLED_Fill(BSP_OLED_t *display, uint8_t pattern);

/**
 * @brief 绘制像素。
 * @param[in,out] display OLED 实例。
 * @param[in] x 横坐标。
 * @param[in] y 纵坐标。
 * @param[in] on 是否点亮。
 */
void BSP_OLED_DrawPixel(BSP_OLED_t *display, int32_t x, int32_t y, bool on);

/**
 * @brief 绘制直线。
 * @param[in,out] display OLED 实例。
 * @param[in] x0 起点横坐标。
 * @param[in] y0 起点纵坐标。
 * @param[in] x1 终点横坐标。
 * @param[in] y1 终点纵坐标。
 * @param[in] on 是否点亮。
 */
void BSP_OLED_DrawLine(BSP_OLED_t *display, int32_t x0, int32_t y0, int32_t x1, int32_t y1, bool on);

/**
 * @brief 绘制矩形。
 * @param[in,out] display OLED 实例。
 * @param[in] x 横坐标。
 * @param[in] y 纵坐标。
 * @param[in] width 宽度。
 * @param[in] height 高度。
 * @param[in] filled 是否填充。
 * @param[in] on 是否点亮。
 */
void BSP_OLED_DrawRect(BSP_OLED_t *display, int32_t x, int32_t y, int32_t width, int32_t height, bool filled, bool on);

/**
 * @brief 绘制位图。
 * @param[in,out] display OLED 实例。
 * @param[in] x 横坐标。
 * @param[in] y 纵坐标。
 * @param[in] bitmap 位图数据。
 * @param[in] width 位图宽度。
 * @param[in] height 位图高度。
 * @param[in] on 是否点亮。
 */
void BSP_OLED_DrawBitmap(BSP_OLED_t *display, int32_t x, int32_t y, const uint8_t *bitmap, int32_t width,
                         int32_t height, bool on);

/**
 * @brief 绘制单个字符。
 * @param[in,out] display OLED 实例。
 * @param[in] x 字符列坐标。
 * @param[in] page 页地址。
 * @param[in] character 字符。
 */
void BSP_OLED_DrawChar(BSP_OLED_t *display, uint8_t x, uint8_t page, char character);

/**
 * @brief 绘制字符串。
 * @param[in,out] display OLED 实例。
 * @param[in] x 字符列坐标。
 * @param[in] page 起始页。
 * @param[in] text 字符串。
 */
void BSP_OLED_PrintString(BSP_OLED_t *display, uint8_t x, uint8_t page, const char *text);

/**
 * @brief 格式化并绘制字符串。
 * @param[in,out] display OLED 实例。
 * @param[in] x 字符列坐标。
 * @param[in] page 起始页。
 * @param[in] format 格式字符串。
 * @param[in] ... 格式参数。
 * @return 格式化长度，失败返回负值。
 */
int BSP_OLED_Printf(BSP_OLED_t *display, uint8_t x, uint8_t page, const char *format, ...);

/**
 * @brief 设置对比度。
 * @param[in,out] display OLED 实例。
 * @param[in] contrast 对比度。
 * @return 成功返回 true，否则返回 false。
 */
bool BSP_OLED_SetContrast(BSP_OLED_t *display, uint8_t contrast);

/**
 * @brief 开启或关闭显示。
 * @param[in,out] display OLED 实例。
 * @param[in] enabled 是否开启。
 * @return 成功返回 true，否则返回 false。
 */
bool BSP_OLED_SetDisplayEnabled(BSP_OLED_t *display, bool enabled);

/**
 * @brief 设置反色显示。
 * @param[in,out] display OLED 实例。
 * @param[in] inverted 是否反色。
 * @return 成功返回 true，否则返回 false。
 */
bool BSP_OLED_SetInverted(BSP_OLED_t *display, bool inverted);

#endif // STM32_OLED_MPU6050_BSP_OLED_H
