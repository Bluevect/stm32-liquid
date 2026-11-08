/**
 * @file app_led.c
 * @brief 板载 LED 状态指示实现。
 */

#include "app_led.h"

#include "bsp_led.h"

/** 正常状态的翻转周期，500 ms 翻转一次即 1 Hz 闪烁。 */
#define APP_LED_OK_PERIOD_MS 500U
/** 故障状态的翻转周期，约 8 Hz 快闪。 */
#define APP_LED_ERR_PERIOD_MS 60U

static uint32_t s_nextToggleMs;
static AppLedState_t s_state = APP_LED_STATE_OK;

/**
 * @brief 取某个状态对应的翻转周期。
 * @param[in] state 指示状态。
 * @return 周期，单位毫秒。
 */
static uint32_t appLedPeriodMs(AppLedState_t state) {
  return (state == APP_LED_STATE_ERROR) ? APP_LED_ERR_PERIOD_MS : APP_LED_OK_PERIOD_MS;
}

void AppLed_Init(uint32_t nowMs) {
  s_state = APP_LED_STATE_OK;
  s_nextToggleMs = nowMs + appLedPeriodMs(s_state);
}

void AppLed_SetState(AppLedState_t state) {
  if (state == s_state) {
    return;
  }

  s_state = state;
  /* 让下一次 Process 立即按新周期翻转，不用等旧周期走完。 */
  s_nextToggleMs = 0U;
}

void AppLed_Process(uint32_t nowMs) {
  if ((int32_t)(nowMs - s_nextToggleMs) < 0) {
    return;
  }

  Led1_Toggle();
  s_nextToggleMs = nowMs + appLedPeriodMs(s_state);
}
