/**
 * @file liquid_probe.c
 * @brief 主机端液体平衡探针：跑指定重力/步数后打印覆盖率与质心。
 *
 * 构建（在项目根目录）：
 *   gcc -std=c11 -O2 -IApp/Inc -IDrivers/BSP/Inc -IDrivers/Platform/Inc \
 *       -Itests/host_hal tests/liquid_probe.c tests/host_hal_stub.c \
 *       App/Src/liquid.c Drivers/BSP/Src/bsp_oled.c -lm -o build/liquid_probe.exe
 *
 * 用法：
 *   build/liquid_probe.exe <gx> <gy> <steps>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp_oled.h"
#include "liquid.h"

static BSP_OLED_t g_display;

static int fb_pixel(int x, int y) {
  if ((x < 0) || (x >= (int)BSP_OLED_WIDTH) || (y < 0) || (y >= (int)BSP_OLED_HEIGHT)) {
    return 0;
  }
  return (g_display.buffer[(y / 8) * (int)BSP_OLED_WIDTH + x] >> (y % 8)) & 1;
}

int main(int argc, char **argv) {
  const float gx = (argc > 1) ? (float)atof(argv[1]) : 0.0f;
  const float gy = (argc > 2) ? (float)atof(argv[2]) : 0.0f;
  const int steps = (argc > 3) ? atoi(argv[3]) : 2000;

  int lit = 0;
  double com_x = 0.0;
  double com_y = 0.0;

  memset(&g_display, 0, sizeof(g_display));
  Liquid_Init();
  Liquid_ResetDemoFill();

  for (int i = 0; i < steps; ++i) {
    Liquid_SetGravity(gx, gy);
    Liquid_Update(0.02f);
  }

  BSP_OLED_Clear(&g_display);
  Liquid_Render(&g_display);

  for (int y = 0; y < (int)BSP_OLED_HEIGHT; ++y) {
    for (int x = 0; x < (int)BSP_OLED_WIDTH; ++x) {
      if (fb_pixel(x, y)) {
        ++lit;
        com_x += x;
        com_y += y;
      }
    }
  }

  printf("g=(%.3f, %.3f) steps=%d lit=%d com=(%.2f, %.2f)\n", gx, gy, steps, lit,
         (lit > 0) ? com_x / lit : -1.0, (lit > 0) ? com_y / lit : -1.0);
  return 0;
}
