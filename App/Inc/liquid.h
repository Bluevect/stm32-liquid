/**
 * @file liquid.h
 * @brief 重力驱动的 2D 液体模拟（渲染到 OLED 显存）。
 *
 * 模型
 * ----
 * 屏幕被切成 LIQUID_GRID_W 列，每列保存液面高度（单位：格，0..LIQUID_GRID_H）。
 * 渲染时把一列展开为 LIQUID_CELL_PX 像素，并在相邻列之间插值，所以水面在像素
 * 级别是平滑的。
 *
 * 静止重力下水面垂直于重力方向，即第 x 列的深度
 *
 *     h(x) = h0 + (gx / |gy|) * x        （裁剪到 [0, H]）
 *
 * h0 由“液体总量守恒”反解得到。实际高度以一阶滞后趋近目标剖面，这就是板子倾斜
 * 时液体“晃一下再静下来”的来源。
 *
 * 重力使用屏幕坐标：
 *   gx > 0 -> 液体向右流，gx < 0 -> 向左流
 *   gy > 0 -> 液体向下流，gy < 0 -> 向上流（板子倒置）
 *   (0, 0) -> 没有面内重力：液体保持不动（板子平放桌面时真实液体也是如此）
 * 每个分量都被限制在 [-1, 1]。
 *
 * 渲染只调用 BSP_OLED_DrawPixel()；清屏和刷屏由调用者负责。
 */

#ifndef STM32_OLED_MPU6050_APP_LIQUID_H
#define STM32_OLED_MPU6050_APP_LIQUID_H

#include <stdint.h>

#include "bsp_oled.h"

/* 模拟网格：32 x 16 格，每格 4 x 4 像素，正好覆盖 128 x 64 屏幕。 */
#define LIQUID_GRID_W 32U
#define LIQUID_GRID_H 16U
#define LIQUID_CELL_PX 4U

/**
 * @brief 复位模拟：清空液体，重力归零。
 */
void Liquid_Init(void);

/**
 * @brief 设置下一次更新使用的重力向量。
 * @param[in] gx 水平重力，-1（左）.. +1（右）。
 * @param[in] gy 垂直重力，-1（上）.. +1（下）。
 */
void Liquid_SetGravity(float gx, float gy);

/**
 * @brief 推进一步模拟。
 * @param[in] dt_s 时间步长，单位秒（内部会做限幅，常用 0.02 s）。
 */
void Liquid_Update(float dt_s);

/**
 * @brief 把当前液面渲染进 OLED 显存。
 * @param[in,out] display OLED 实例。
 */
void Liquid_Render(BSP_OLED_t *display);

/**
 * @brief 把模拟置为确定的演示状态（装约 45% 的液体）。
 */
void Liquid_ResetDemoFill(void);

#endif // STM32_OLED_MPU6050_APP_LIQUID_H
