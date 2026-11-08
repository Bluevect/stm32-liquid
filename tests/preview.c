/**
 * @file preview.c
 * @brief 主机端液体模拟渲染器（把固件同一份 liquid.c + bsp_oled.c 渲染成 PGM）。
 *
 * 构建（在项目根目录，或直接运行 tests/run_host_tests.ps1）：
 *   gcc -std=c11 -O2 -Wall -IApp/Inc -IDrivers/BSP/Inc -IDrivers/Platform/Inc \
 *       -Itests/host_hal tests/preview.c tests/host_hal_stub.c \
 *       App/Src/liquid.c Drivers/BSP/Src/bsp_oled.c -lm -o build/preview.exe
 *
 * 用法：
 *   build/preview.exe contact   -> build/preview/frameNN_<label>.pgm（6 张静态姿势）
 *   build/preview.exe anim      -> build/preview/anim/animNNN.pgm（8 s @ 25 fps）
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "bsp_oled.h"
#include "liquid.h"

#define PREVIEW_DIR "build/preview"

static BSP_OLED_t g_display;

static void write_pgm(const char *path) {
  FILE *file = fopen(path, "wb");

  if (file == NULL) {
    printf("cannot write %s\n", path);
    return;
  }

  fprintf(file, "P5\n%u %u\n1\n", BSP_OLED_WIDTH, BSP_OLED_HEIGHT);
  for (uint32_t y = 0U; y < BSP_OLED_HEIGHT; ++y) {
    for (uint32_t x = 0U; x < BSP_OLED_WIDTH; ++x) {
      const uint8_t bit = (uint8_t)((g_display.buffer[(y / 8U) * BSP_OLED_WIDTH + x] >> (y % 8U)) & 1U);

      fputc(bit ? 1 : 0, file);
    }
  }
  fclose(file);
}

/* 与固件 app_oled.c 相同的 HUD 版式：顶部两页黑底 + 文本。 */
static void compose(float gx, float gy, const char *status) {
  char line1[20];
  char line2[20];
  const int32_t pitch = (int32_t)(asinf(fmaxf(-1.0f, fminf(1.0f, gx))) * 57.29578f);
  const int32_t roll = (int32_t)(asinf(fmaxf(-1.0f, fminf(1.0f, gy))) * 57.29578f);

  snprintf(line1, sizeof(line1), "P%+ld R%+ld", (long)pitch, (long)roll);
  snprintf(line2, sizeof(line2), "X%+ld Y%+ld", (long)(gx * 100.0f), (long)(gy * 100.0f));

  BSP_OLED_Clear(&g_display);
  Liquid_Render(&g_display);
  BSP_OLED_DrawRect(&g_display, 0, 0, (int32_t)BSP_OLED_WIDTH, 16, true, false);
  BSP_OLED_PrintString(&g_display, 0U, 0U, line1);
  BSP_OLED_PrintString(&g_display, 0U, 1U, line2);
  BSP_OLED_PrintString(&g_display, (uint8_t)(BSP_OLED_WIDTH - 6U * strlen(status)), 0U, status);
}

static void settle(float gx, float gy, int steps) {
  for (int i = 0; i < steps; ++i) {
    Liquid_SetGravity(gx, gy);
    Liquid_Update(0.02f);
  }
}

static void contact_sheet(void) {
  static const struct {
    float gx;
    float gy;
    const char *label;
    const char *status;
  } poses[] = {
      {0.000f, 0.020f, "flat_on_table", "OK"}, {0.170f, 0.985f, "tilt_10deg", "OK"},
      {0.500f, 0.870f, "tilt_30deg", "OK"},    {1.000f, 0.050f, "rolled_90deg", "OK"},
      {0.000f, -1.000f, "upside_down", "OK"},  {-0.500f, 0.870f, "tilt_left_30", "OK"},
  };

  for (size_t i = 0U; i < (sizeof(poses) / sizeof(poses[0])); ++i) {
    char path[128];

    Liquid_Init();
    Liquid_ResetDemoFill();
    settle(poses[i].gx, poses[i].gy, 2500);
    compose(poses[i].gx, poses[i].gy, poses[i].status);
    snprintf(path, sizeof(path), PREVIEW_DIR "/frame%02u_%s.pgm", (unsigned)(i + 1U), poses[i].label);
    write_pgm(path);
    printf("wrote %s\n", path);
  }

  {
    FILE *list = fopen(PREVIEW_DIR "/frames.txt", "w");

    if (list != NULL) {
      for (size_t i = 0U; i < (sizeof(poses) / sizeof(poses[0])); ++i) {
        fprintf(list, "%s\n", poses[i].label);
      }
      fclose(list);
    }
  }
}

static void animation(void) {
  const int frames = 200;
  const float dt = 0.04f; /* 25 fps */

  Liquid_Init();
  Liquid_ResetDemoFill();

  for (int frame = 0; frame < frames; ++frame) {
    const float t = (float)frame * dt;
    const float gx = 0.55f * sinf(t * 0.90f);
    const float gy = 0.65f + 0.25f * sinf(t * 0.55f + 1.00f);
    char path[160];

    settle(gx, gy, 1);
    compose(gx, gy, "DEMO");
    snprintf(path, sizeof(path), PREVIEW_DIR "/anim/anim%03d.pgm", frame);
    write_pgm(path);
  }
  printf("wrote %d animation frames to %s/anim\n", frames, PREVIEW_DIR);
}

int main(int argc, char **argv) {
  const char *mode = (argc > 1) ? argv[1] : "contact";

  memset(&g_display, 0, sizeof(g_display));

  if (strcmp(mode, "anim") == 0) {
    animation();
  } else {
    contact_sheet();
  }
  return 0;
}
