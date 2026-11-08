/**
 * @file app_oled.c
 * @brief OLED 显示层：液体动画 + 顶部 HUD。
 *
 * 与 FreeRTOS 版本的区别：去掉了 displayTask 和 imuQueue，改为由
 * AppMain_Loop() 按固定周期调用 AppOled_Refresh()，直接读取 AppImu 的最新样本。
 */

#include "app_oled.h"

#include <stddef.h>
#include <string.h>

#include "app_imu.h"
#include "liquid.h"

/** 顶部 HUD 黑底高度（像素），正好两页。 */
#define APP_HUD_HEIGHT 16

/** HUD 每行文本的最大长度。 */
#define APP_HUD_LINE_LEN 22U

static BSP_OLED_t *s_display;
static bool s_oledReady;
static char s_hudLine1[APP_HUD_LINE_LEN];
static char s_hudLine2[APP_HUD_LINE_LEN];
static const char *s_statusText = "OK";

/**
 * @brief 追加一个带符号、至少 3 位的十进制数。
 * @param[out] p 写入位置。
 * @param[in] v 数值。
 * @return 新的写入位置。
 * @note 不引入 printf，保持代码体积和速度可控。
 */
static char *appAppendSigned(char *p, int32_t v) {
  if (v < 0) {
    *p++ = '-';
    v = -v;
  } else {
    *p++ = '+';
  }

  if (v >= 100) {
    *p++ = (char)('0' + ((v / 100) % 10));
  }
  if (v >= 10) {
    *p++ = (char)('0' + ((v / 10) % 10));
  }
  *p++ = (char)('0' + (v % 10));
  return p;
}

/**
 * @brief 把浮点重力转换为百分数整数（四舍五入，NaN 记为 0）。
 * @param[in] value 浮点值。
 * @return 百分数整数。
 */
static int32_t appPercent(float value) {
  if (!(value == value)) {
    return 0;
  }
  return (int32_t)((value >= 0.0f) ? (value * 100.0f + 0.5f) : (value * 100.0f - 0.5f));
}

/**
 * @brief 生成两行 HUD 文本。
 */
static void appBuildHud(void) {
  const ImuSample_t *sample = AppImu_Latest();
  const AppImuStatus_t status = AppImu_Status();
  char *p;

  if (status == APP_IMU_STATUS_DEMO) {
    s_statusText = "DEMO";
  } else if ((status == APP_IMU_STATUS_ERROR) || !sample->valid) {
    s_statusText = "ERR";
  } else {
    s_statusText = "OK";
  }

  p = s_hudLine1;
  *p++ = 'P';
  p = appAppendSigned(p, sample->valid ? (sample->pitchDegX100 / 100) : 0);
  *p++ = ' ';
  *p++ = 'R';
  p = appAppendSigned(p, sample->valid ? (sample->rollDegX100 / 100) : 0);
  *p = '\0';

  p = s_hudLine2;
  *p++ = 'X';
  p = appAppendSigned(p, appPercent(sample->gravityX));
  *p++ = ' ';
  *p++ = 'Y';
  p = appAppendSigned(p, appPercent(sample->gravityY));
  *p = '\0';
}

/**
 * @brief 初始化 OLED 并显示启动画面。
 * @param[out] display OLED 实例。
 * @param[in] i2cBus 所属 I2C 总线。
 * @return 初始化成功返回 true，否则返回 false。
 */
bool AppOled_Init(BSP_OLED_t *display, I2C_Bus_t *i2cBus) {
  s_display = display;
  s_oledReady = false;

  if ((display == NULL) || (i2cBus == NULL)) {
    return false;
  }

  if (!BSP_OLED_Init(display, i2cBus)) {
    return false;
  }

  BSP_OLED_Clear(display);
  BSP_OLED_PrintString(display, 22U, 2U, "LIQUID GRAVITY");
  BSP_OLED_PrintString(display, 22U, 3U, "TILT THE BOARD");
  BSP_OLED_Display(display);
  HAL_Delay(800U);

  s_oledReady = true;
  return true;
}

/**
 * @brief 刷新一帧：液体 + HUD 文本，然后推送到屏幕。
 * @param[in] nowMs 当前毫秒时刻（保留给后续动画使用）。
 */
void AppOled_Refresh(uint32_t nowMs) {
  const uint8_t statusWidth = 6U;

  (void)nowMs;

  if (!s_oledReady || (s_display == NULL)) {
    return;
  }

  appBuildHud();

  BSP_OLED_Clear(s_display);
  Liquid_Render(s_display);

  /* 顶部两页铺黑底，保证 HUD 在液体上也清晰可读。 */
  BSP_OLED_DrawRect(s_display, 0, 0, (int32_t)BSP_OLED_WIDTH, APP_HUD_HEIGHT, true, false);
  BSP_OLED_PrintString(s_display, 0U, 0U, s_hudLine1);
  BSP_OLED_PrintString(s_display, 0U, 1U, s_hudLine2);
  BSP_OLED_PrintString(s_display, (uint8_t)(BSP_OLED_WIDTH - statusWidth * strlen(s_statusText)), 0U,
                       s_statusText);

  s_oledReady = BSP_OLED_Display(s_display);
}

/**
 * @brief 查询 OLED 是否可用。
 * @return 可用返回 true。
 */
bool AppOled_IsReady(void) { return s_oledReady; }
