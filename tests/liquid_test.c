/**
 * @file liquid_test.c
 * @brief 液体重力模拟的主机端行为测试（与固件共用同一份 liquid.c / bsp_oled.c）。
 *
 * 构建与运行（在项目根目录）：
 *   powershell -ExecutionPolicy Bypass -File tests/run_host_tests.ps1
 * 或者手动：
 *   gcc -std=c11 -O1 -Wall -Wextra -IApp/Inc -IDrivers/BSP/Inc \
 *       -IDrivers/Platform/Inc -Itests/host_hal \
 *       tests/liquid_test.c tests/host_hal_stub.c App/Src/liquid.c \
 *       Drivers/BSP/Src/bsp_oled.c -lm -o build/liquid_test.exe
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "bsp_oled.h"
#include "liquid.h"

static BSP_OLED_t g_display;

static int g_failures;
static int g_checks;

#define CHECK(cond, fmt, ...)                         \
  do {                                                \
    ++g_checks;                                       \
    if (!(cond)) {                                    \
      ++g_failures;                                   \
      printf("  FAIL  " fmt "\n", ##__VA_ARGS__);     \
    } else {                                          \
      printf("  ok    " fmt "\n", ##__VA_ARGS__);     \
    }                                                 \
  } while (0)

/* ========================================================================== */
/* 显存辅助                                                                   */
/* ========================================================================== */

static int fb_pixel(int x, int y) {
  if ((x < 0) || (x >= (int)BSP_OLED_WIDTH) || (y < 0) || (y >= (int)BSP_OLED_HEIGHT)) {
    return 0;
  }
  return (g_display.buffer[(y / 8) * (int)BSP_OLED_WIDTH + x] >> (y % 8)) & 1;
}

static int fb_lit_count(void) {
  int count = 0;

  for (int y = 0; y < (int)BSP_OLED_HEIGHT; ++y) {
    for (int x = 0; x < (int)BSP_OLED_WIDTH; ++x) {
      count += fb_pixel(x, y);
    }
  }
  return count;
}

static double fb_com(int axis) {
  double sum = 0.0;
  int count = 0;

  for (int y = 0; y < (int)BSP_OLED_HEIGHT; ++y) {
    for (int x = 0; x < (int)BSP_OLED_WIDTH; ++x) {
      if (fb_pixel(x, y)) {
        sum += (axis == 0) ? x : y;
        ++count;
      }
    }
  }
  return (count > 0) ? (sum / count) : -1.0;
}

static int fb_hash(void) {
  unsigned int hash = 2166136261U;

  for (int page = 0; page < (int)BSP_OLED_PAGES; ++page) {
    for (int x = 0; x < (int)BSP_OLED_WIDTH; ++x) {
      hash ^= g_display.buffer[page * (int)BSP_OLED_WIDTH + x];
      hash *= 16777619U;
    }
  }
  return (int)(hash & 0x7FFFFFFFU);
}

static void render(void) {
  BSP_OLED_Clear(&g_display);
  Liquid_Render(&g_display);
}

static void run_steps(float gx, float gy, int steps) {
  for (int i = 0; i < steps; ++i) {
    Liquid_SetGravity(gx, gy);
    Liquid_Update(0.02f);
  }
}

/* 确定性伪随机 [-1, 1]。 */
static float next_gravity(unsigned int *state) {
  *state = (*state * 1103515245U + 12345U);
  return ((float)((*state >> 8) & 0xFFFFU) / 32767.5f) - 1.0f;
}

static int top_half_lit(void) {
  int count = 0;

  for (int y = 0; y < (int)BSP_OLED_HEIGHT / 2; ++y) {
    for (int x = 0; x < (int)BSP_OLED_WIDTH; ++x) {
      count += fb_pixel(x, y);
    }
  }
  return count;
}

static int bottom_row_lit(void) {
  int count = 0;

  for (int x = 0; x < (int)BSP_OLED_WIDTH; ++x) {
    count += fb_pixel(x, (int)BSP_OLED_HEIGHT - 1);
  }
  return count;
}

static void column_surface(int *surface) {
  for (int x = 0; x < (int)BSP_OLED_WIDTH; ++x) {
    surface[x] = -1;
    for (int y = 0; y < (int)BSP_OLED_HEIGHT; ++y) {
      if (fb_pixel(x, y)) {
        surface[x] = y;
        break;
      }
    }
  }
}

/* ========================================================================== */
/* DrawPixel 语义                                                             */
/* ========================================================================== */

static void test_drawpixel(void) {
  printf("[BSP_OLED_DrawPixel]\n");
  BSP_OLED_Clear(&g_display);
  CHECK(fb_lit_count() == 0, "clear leaves an empty framebuffer");

  /* 左上角 = page 0, bit 0。 */
  BSP_OLED_DrawPixel(&g_display, 0, 0, true);
  CHECK(g_display.buffer[0] == 0x01U, "pixel (0,0) sets page 0 bit 0 (got 0x%02X)", g_display.buffer[0]);

  /* 右下角 = page 7, bit 7。 */
  BSP_OLED_DrawPixel(&g_display, (int32_t)BSP_OLED_WIDTH - 1, (int32_t)BSP_OLED_HEIGHT - 1, true);
  CHECK(g_display.buffer[7 * BSP_OLED_WIDTH + BSP_OLED_WIDTH - 1] == 0x80U,
        "pixel (127,63) sets page 7 bit 7");

  /* 页内位序：y = 9 -> page 1, bit 1。 */
  BSP_OLED_DrawPixel(&g_display, 5, 9, true);
  CHECK((g_display.buffer[1 * BSP_OLED_WIDTH + 5] & 0xFFU) == 0x02U, "pixel (5,9) sets page 1 bit 1");

  /* on = false 能擦除。 */
  BSP_OLED_DrawPixel(&g_display, 0, 0, false);
  CHECK(fb_pixel(0, 0) == 0, "pixel (0,0) can be cleared with on=false");

  /* 越界坐标必须被忽略而不是写坏显存。 */
  {
    static uint8_t snapshot[BSP_OLED_BUFFER_SIZE];

    memcpy(snapshot, g_display.buffer, sizeof(snapshot));
    BSP_OLED_DrawPixel(&g_display, -1, 0, true);
    BSP_OLED_DrawPixel(&g_display, (int32_t)BSP_OLED_WIDTH, 0, true);
    BSP_OLED_DrawPixel(&g_display, 0, -1, true);
    BSP_OLED_DrawPixel(&g_display, 0, (int32_t)BSP_OLED_HEIGHT, true);
    CHECK(memcmp(snapshot, g_display.buffer, sizeof(snapshot)) == 0, "out-of-range pixels are ignored");
  }
}

/* ========================================================================== */
/* 液体行为                                                                   */
/* ========================================================================== */

static void test_empty(void) {
  printf("[empty grid]\n");
  Liquid_Init();
  render();
  CHECK(fb_lit_count() == 0, "no pixels drawn on an empty grid (got %d)", fb_lit_count());
}

static void test_gravity_down(void) {
  printf("[gravity down]\n");
  Liquid_Init();
  Liquid_ResetDemoFill();
  run_steps(0.0f, 1.0f, 300);
  render();
  CHECK(fb_lit_count() > 100, "liquid is visible (%d pixels)", fb_lit_count());
  CHECK(fb_com(1) > 40.0, "centre of mass moves to the bottom (com_y=%.1f)", fb_com(1));
  CHECK(fb_com(1) < 64.0, "centre of mass stays on screen (com_y=%.1f)", fb_com(1));
  CHECK(bottom_row_lit() > 40, "liquid rests on the bottom edge (%d px in last row)", bottom_row_lit());
}

static void test_gravity_right(void) {
  printf("[gravity right]\n");
  Liquid_Init();
  Liquid_ResetDemoFill();
  run_steps(1.0f, 0.0f, 500);
  render();
  CHECK(fb_com(0) > 80.0, "liquid flows to the right (com_x=%.1f)", fb_com(0));
  CHECK(fb_lit_count() > 100, "liquid is still visible (%d pixels)", fb_lit_count());
}

static void test_gravity_left(void) {
  printf("[gravity left]\n");
  Liquid_Init();
  Liquid_ResetDemoFill();
  run_steps(-1.0f, 0.0f, 500);
  render();
  CHECK(fb_com(0) < 48.0, "liquid flows to the left (com_x=%.1f)", fb_com(0));
}

static void test_gravity_up(void) {
  printf("[gravity up]\n");
  Liquid_Init();
  Liquid_ResetDemoFill();
  run_steps(0.0f, -1.0f, 600);
  render();
  CHECK(fb_com(1) < 32.0, "liquid flows to the top when the board is inverted (com_y=%.1f)", fb_com(1));
}

static void test_flat_surface(void) {
  printf("[level surface when flat]\n");
  Liquid_Init();
  Liquid_ResetDemoFill();
  run_steps(0.0f, 1.0f, 1500);
  render();
  {
    int surface[128];
    int min_top = 999;
    int max_top = -1;

    column_surface(surface);
    for (int x = 0; x < (int)BSP_OLED_WIDTH; ++x) {
      if (surface[x] > 0) {
        min_top = (surface[x] < min_top) ? surface[x] : min_top;
        max_top = (surface[x] > max_top) ? surface[x] : max_top;
      }
    }
    CHECK(min_top >= 0 && max_top >= 0, "surface found (min=%d max=%d)", min_top, max_top);
    CHECK((max_top - min_top) <= 12, "surface is flat within 12 px (spread=%d px)", max_top - min_top);
  }
}

static void test_weightless_hold(void) {
  printf("[no in-plane gravity keeps the puddle]\n");
  Liquid_Init();
  Liquid_ResetDemoFill();
  render();
  {
    const int lit_before = fb_lit_count();
    const int top_before = top_half_lit();

    run_steps(0.0f, 0.0f, 2500);
    render();
    CHECK(top_half_lit() < 100, "liquid does not creep to the top half (%d -> %d px)", top_before,
          top_half_lit());
    CHECK(fb_lit_count() > lit_before * 8 / 10, "puddle keeps its body instead of spreading thin (%d -> %d px)",
          lit_before, fb_lit_count());
    CHECK(fb_com(1) > 40.0, "puddle stays at the bottom (com_y=%.1f)", fb_com(1));
  }
}

static void test_small_tilt_not_diluted(void) {
  printf("[10 deg tilt: puddle moves but stays compact]\n");
  Liquid_Init();
  Liquid_ResetDemoFill();
  run_steps(0.0f, 1.0f, 400);
  render();
  {
    const double com_x0 = fb_com(0);
    const int lit0 = fb_lit_count();

    run_steps(0.17f, 0.985f, 1500);
    render();
    CHECK(fb_com(0) > com_x0 + 8.0, "liquid slides towards the low side (com_x %.1f -> %.1f)", com_x0,
          fb_com(0));
    CHECK(fb_lit_count() > lit0 * 8 / 10, "liquid is not diluted into a haze (%d -> %d px)", lit0,
          fb_lit_count());
  }
}

static void test_tilted_surface_holds(void) {
  printf("[tilted free surface stays tilted]\n");
  Liquid_Init();
  Liquid_ResetDemoFill();
  run_steps(0.5f, 0.87f, 2000);
  render();
  {
    const double com_x1 = fb_com(0);
    const double com_y1 = fb_com(1);
    const int lit1 = fb_lit_count();

    run_steps(0.5f, 0.87f, 2000);
    render();
    CHECK(fabs(fb_com(0) - com_x1) < 3.0, "surface does not creep sideways (%.1f -> %.1f)", com_x1, fb_com(0));
    CHECK(fabs(fb_com(1) - com_y1) < 3.0, "surface does not creep vertically (%.1f -> %.1f)", com_y1, fb_com(1));
    CHECK(fb_lit_count() > lit1 * 8 / 10 && fb_lit_count() < lit1 * 12 / 10, "puddle keeps its body (%d -> %d px)",
          lit1, fb_lit_count());
    CHECK(com_x1 > 72.0, "liquid piles on the low side (com_x=%.1f)", com_x1);
  }
}

static void test_bounds_and_stability(void) {
  printf("[bounds / stability / no NaN]\n");
  Liquid_Init();
  Liquid_ResetDemoFill();
  {
    unsigned int state = 0x12345678U;

    for (int frame = 0; frame < 60; ++frame) {
      for (int i = 0; i < 20; ++i) {
        Liquid_SetGravity(next_gravity(&state), next_gravity(&state));
        Liquid_Update(0.02f);
      }
      render();
      if (fb_lit_count() == 0) {
        break;
      }
    }
    CHECK(fb_lit_count() > 0, "simulation did not collapse to nothing (%d pixels)", fb_lit_count());
    CHECK(fb_com(0) >= 0.0 && fb_com(0) <= 127.0 && fb_com(1) >= 0.0 && fb_com(1) <= 63.0,
          "centre of mass stays inside the panel (%.1f, %.1f)", fb_com(0), fb_com(1));
  }
}

static void test_area_conservation(void) {
  printf("[area proxy for mass conservation]\n");
  Liquid_Init();
  Liquid_ResetDemoFill();
  run_steps(0.0f, 1.0f, 300);
  render();
  {
    const int area0 = fb_lit_count();
    unsigned int state = 0xC0FFEEU;

    for (int i = 0; i < 2000; ++i) {
      Liquid_SetGravity(next_gravity(&state) * 0.5f, next_gravity(&state) * 0.5f + 0.5f);
      Liquid_Update(0.02f);
    }
    render();
    CHECK(fb_lit_count() > area0 * 6 / 10, "rendered liquid area is preserved (%d -> %d px)", area0,
          fb_lit_count());
    CHECK(fb_lit_count() < area0 * 14 / 10, "rendered liquid area did not explode (%d -> %d px)", area0,
          fb_lit_count());
  }
}

static void test_determinism(void) {
  printf("[determinism]\n");
  Liquid_Init();
  Liquid_ResetDemoFill();
  run_steps(0.3f, 0.9f, 400);
  render();
  {
    const int hash1 = fb_hash();

    Liquid_Init();
    Liquid_ResetDemoFill();
    run_steps(0.3f, 0.9f, 400);
    render();
    CHECK(hash1 == fb_hash(), "same inputs give the same frame (%d vs %d)", hash1, fb_hash());
    CHECK(hash1 != 0, "frame is not empty");
  }
}

int main(void) {
  memset(&g_display, 0, sizeof(g_display));

  printf("=== liquid simulation tests ===\n");
  test_drawpixel();
  test_empty();
  test_gravity_down();
  test_gravity_right();
  test_gravity_left();
  test_gravity_up();
  test_flat_surface();
  test_weightless_hold();
  test_small_tilt_not_diluted();
  test_tilted_surface_holds();
  test_bounds_and_stability();
  test_area_conservation();
  test_determinism();

  printf("=== %d checks, %d failure(s) ===\n", g_checks, g_failures);
  return (g_failures == 0) ? 0 : 1;
}
