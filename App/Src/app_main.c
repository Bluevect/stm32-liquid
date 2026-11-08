/**
 * @file app_main.c
 * @brief 应用层初始化、共享实例和裸机超级循环。
 *
 * 与 FreeRTOS 版本的区别
 * ----------------------
 * 原工程用 imuTask + displayTask + imuQueue 做任务间通信。这里完全去掉 RTOS：
 * main() 里只有一个超级循环，三类周期任务（IMU 轮询、液体物理、OLED 刷新）
 * 都由 HAL_GetTick() 驱动的软件定时器调度，各任务之间通过模块内的“最新样本”
 * 共享数据，不存在并发访问，因此也不需要互斥锁。
 */

#include "app_main.h"

#include <stddef.h>

#include "app_imu.h"
#include "app_led.h"
#include "app_oled.h"
#include "app_utils.h"
#include "i2c.h"
#include "i2c_hardware.h"
#include "liquid.h"
#include "usart.h"

/** MPU6050 FIFO 轮询周期（毫秒），应明显快于 DMP 的 50 Hz 输出。 */
#define APP_IMU_POLL_PERIOD_MS 5U
/** 液体物理步长（毫秒）。 */
#define APP_LIQUID_PERIOD_MS 20U
/** OLED 刷新周期（毫秒）。 */
#define APP_DISPLAY_PERIOD_MS 33U

BSP_USART_t usartInstance;
BSP_OLED_t oledInstance;
MPU6050_t mpu6050Instance;

static I2C_Bus_t s_i2c1Bus;
static I2C_Bus_t s_i2c2Bus;

static AppUtils_Periodic_t s_imuPollTimer;
static AppUtils_Periodic_t s_liquidTimer;
static AppUtils_Periodic_t s_displayTimer;
static uint32_t s_lastLiquidMs;
static bool s_oledReady;

/**
 * @brief 把串口收到的数据原样回传。
 * @param[in,out] usart 串口实例。
 * @param[in] data 接收数据。
 * @param[in] size 数据长度。
 */
static void appUartFrameReceived(BSP_USART_t *usart, const uint8_t *data, uint16_t size) {
  BSP_USART_Transmit(usart, data, size);
}

/**
 * @brief 把最新重力喂给液体模拟并推进一步。
 * @param[in] nowMs 当前毫秒时刻。
 */
static void appStepLiquid(uint32_t nowMs) {
  const ImuSample_t *sample = AppImu_Latest();
  const uint32_t elapsedMs = AppUtils_ElapsedMs(nowMs, s_lastLiquidMs);
  float dt = (float)elapsedMs / 1000.0f;

  s_lastLiquidMs = nowMs;
  if (sample->valid) {
    Liquid_SetGravity(sample->gravityX, sample->gravityY);
  }
  Liquid_Update(dt);
}

bool AppMain_Init(void) {
  uint32_t now;

  if (!I2C_Bus_Init(&s_i2c1Bus, &hi2c1) || !I2C_Bus_Init(&s_i2c2Bus, &hi2c2)) {
    return false;
  }

  if (!BSP_USART_Init(&usartInstance, &huart1, appUartFrameReceived)) {
    return false;
  }

  s_oledReady = AppOled_Init(&oledInstance, &s_i2c2Bus);
  (void)AppImu_Init(&mpu6050Instance, &s_i2c1Bus);  /* 失败自动进入 DEMO 模式 */
  Liquid_Init();
  Liquid_ResetDemoFill();

  now = HAL_GetTick();
  AppUtils_PeriodicInit(&s_imuPollTimer, now, APP_IMU_POLL_PERIOD_MS);
  AppUtils_PeriodicInit(&s_liquidTimer, now, APP_LIQUID_PERIOD_MS);
  AppUtils_PeriodicInit(&s_displayTimer, now, APP_DISPLAY_PERIOD_MS);
  s_lastLiquidMs = now;

  AppLed_Init(now);
  AppLed_SetState(s_oledReady ? APP_LED_STATE_OK : APP_LED_STATE_ERROR);
  return true;
}

void AppMain_Loop(void) {
  const uint32_t now = HAL_GetTick();

  AppLed_Process(now);

  if (AppUtils_PeriodicDue(&s_imuPollTimer, now)) {
    AppImu_Poll(now);
  }
  if (AppUtils_PeriodicDue(&s_liquidTimer, now)) {
    appStepLiquid(now);
  }
  if (AppUtils_PeriodicDue(&s_displayTimer, now)) {
    AppOled_Refresh(now);
  }
}
