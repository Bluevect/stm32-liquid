/**
 * @file app_imu.c
 * @brief IMU 数据采集与重力映射（裸机轮询版）。
 *
 * 与 FreeRTOS 版本的区别：去掉了 imuTask / imuQueue，改为由 AppMain_Loop()
 * 按固定周期调用 AppImu_Poll()，结果放在模块内的单份最新样本里，显示层直接
 * 读取。这样没有任务切换、没有队列，也没有任何 RTOS 依赖。
 */

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "app_imu.h"
#include "app_utils.h"
#include "bsp_mpu6050.h"

/* ========================================================================== */
/* 可调参数                                                                   */
/* ========================================================================== */

/** 连续读取失败多少次后报告 ERR。 */
#define APP_IMU_ERR_LIMIT 20U
/** 多久没有新样本就认为传感器掉线（毫秒）。 */
#define APP_IMU_STALE_TIMEOUT_MS 2000U
/** 一阶低通滤波系数（0 = 冻结，1 = 不滤波）。 */
#define APP_IMU_FILTER_ALPHA 0.25f
/** 重力死区，避免板子放平时噪声让液体缓慢漂移。 */
#define APP_IMU_GRAVITY_DEADZONE 0.02f

/* 传感器安装方向标定。
 *
 * 实机核对结论：MPU6050 相对屏幕旋转了 90°，表现为"动 Pitch 时液体左右动、
 * 动 Roll 时液体上下动"。因此需要交换两个重力轴（SWAP_XY = 1），让：
 *   - Pitch（绕传感器 Y 轴）-> 屏幕竖直方向
 *   - Roll （绕传感器 X 轴）-> 屏幕水平方向
 *
 * 交换后再按屏幕方向取符号（实机核对：左右正确、上下相反，故 y 取负）：
 *   screen_x = -gravity_y
 *   screen_y = -gravity_x
 *
 * 若换板或传感器换安装方向后出现某个轴方向相反，只改对应的 SIGN 即可。 */
#define APP_GRAVITY_SIGN_X (-1.0f)
#define APP_GRAVITY_SIGN_Y (-1.0f)
#define APP_GRAVITY_SWAP_XY (1)

/* DEMO 模式合成重力（没有 MPU6050 时也能看液体动画）。 */
#define APP_DEMO_AMP_X 0.55f
#define APP_DEMO_AMP_Y 0.25f
#define APP_DEMO_BIAS_Y 0.65f
#define APP_DEMO_RATE_X 0.90f
#define APP_DEMO_RATE_Y 0.55f
#define APP_DEMO_PHASE_Y 1.00f

#define APP_RAD_TO_DEG 57.29577951f

/* ========================================================================== */
/* 模块状态                                                                   */
/* ========================================================================== */

static MPU6050_t *s_device;
static AppImuStatus_t s_status = APP_IMU_STATUS_DEMO;
static ImuSample_t s_sample;
static float s_gravityX;
static float s_gravityY;
static uint32_t s_lastSampleMs;
static uint32_t s_errorStreak;
static uint32_t s_demoStartMs;
static bool s_demoMode = true;

/* ========================================================================== */
/* 内部函数                                                                   */
/* ========================================================================== */

/**
 * @brief 将板体系重力映射到屏幕坐标，并做死区与幅度限制。
 * @param[in] gx 板体系 x 分量。
 * @param[in] gy 板体系 y 分量。
 * @param[out] outX 屏幕 x 方向重力。
 * @param[out] outY 屏幕 y 方向重力。
 */
static void appImuMapGravity(float gx, float gy, float *outX, float *outY) {
  float mag;

#if APP_GRAVITY_SWAP_XY
  {
    const float swap = gx;
    gx = gy;
    gy = swap;
  }
#endif

  gx = APP_GRAVITY_SIGN_X * gx;
  gy = APP_GRAVITY_SIGN_Y * gy;
  mag = sqrtf(gx * gx + gy * gy);
  if (mag < APP_IMU_GRAVITY_DEADZONE) {
    gx = 0.0f;
    gy = 0.0f;
  } else if (mag > 1.0f) {
    gx /= mag;
    gy /= mag;
  }

  if (gx != gx) { /* NaN 保护 */
    gx = 0.0f;
  }
  if (gy != gy) {
    gy = 0.0f;
  }

  *outX = gx;
  *outY = gy;
}

/**
 * @brief 用合成重力填充样本，保持 DEMO 模式下的动画和读数都合理。
 * @param[in] nowMs 当前毫秒时刻。
 */
static void appImuUpdateDemo(uint32_t nowMs) {
  const float t = (float)(nowMs - s_demoStartMs) / 1000.0f;
  float gx = APP_DEMO_AMP_X * sinf(t * APP_DEMO_RATE_X);
  float gy = APP_DEMO_BIAS_Y + APP_DEMO_AMP_Y * sinf(t * APP_DEMO_RATE_Y + APP_DEMO_PHASE_Y);

  gx = (gx > 1.0f) ? 1.0f : ((gx < -1.0f) ? -1.0f : gx);
  gy = (gy > 1.0f) ? 1.0f : ((gy < -1.0f) ? -1.0f : gy);

  s_gravityX = gx;
  s_gravityY = gy;
  s_sample.gravityX = gx;
  s_sample.gravityY = gy;
  s_sample.pitchDegX100 = (int32_t)(asinf(gx) * APP_RAD_TO_DEG * 100.0f);
  s_sample.rollDegX100 = (int32_t)(asinf(gy) * APP_RAD_TO_DEG * 100.0f);
  s_sample.yawDegX100 = 0;
  s_sample.timestampMs = nowMs;
  s_sample.valid = true;
}

/* ========================================================================== */
/* 公共接口                                                                   */
/* ========================================================================== */

bool AppImu_Init(MPU6050_t *device, I2C_Bus_t *i2cBus) {
  memset(&s_sample, 0, sizeof(s_sample));
  s_gravityX = 0.0f;
  s_gravityY = 0.0f;
  s_errorStreak = 0U;
  s_demoStartMs = HAL_GetTick();
  s_lastSampleMs = s_demoStartMs;
  s_device = device;

  if ((device != NULL) && (i2cBus != NULL) && MPU6050_InitDmp(device, i2cBus)) {
    s_demoMode = false;
    s_status = APP_IMU_STATUS_OK;
    return true;
  }

  s_demoMode = true;
  s_status = APP_IMU_STATUS_DEMO;
  return false;
}

void AppImu_Poll(uint32_t nowMs) {
  MPU6050_DmpData_t data;

  if (s_demoMode) {
    appImuUpdateDemo(nowMs);
    return;
  }

  {
    const MPU6050_DmpStatus_t dmpStatus = MPU6050_ReadDmp(s_device, &data);

    if (dmpStatus == MPU6050_DMP_DATA_READY) {
      float mappedX;
      float mappedY;

      s_errorStreak = 0U;
      s_lastSampleMs = nowMs;
      s_status = APP_IMU_STATUS_OK;

      /* 一阶低通：滤掉 DMP 输出的高频抖动，同时不引入明显滞后。 */
      s_gravityX += APP_IMU_FILTER_ALPHA * (data.gravityX - s_gravityX);
      s_gravityY += APP_IMU_FILTER_ALPHA * (data.gravityY - s_gravityY);
      appImuMapGravity(s_gravityX, s_gravityY, &mappedX, &mappedY);

      s_sample.gravityX = mappedX;
      s_sample.gravityY = mappedY;
      s_sample.rollDegX100 = data.rollDegX100;
      s_sample.pitchDegX100 = data.pitchDegX100;
      s_sample.yawDegX100 = data.yawDegX100;
      s_sample.timestampMs = nowMs;
      s_sample.valid = true;
    } else if (dmpStatus == MPU6050_DMP_ERROR) {
      if (s_errorStreak < 0xFFFFFFFFU) {
        ++s_errorStreak;
      }
      if (s_errorStreak >= APP_IMU_ERR_LIMIT) {
        s_status = APP_IMU_STATUS_ERROR;
      }
    }

    /* 初始化成功但一直没有数据（接线/ FIFO 故障）：退回 DEMO，让屏幕说话。 */
    if ((uint32_t)(nowMs - s_lastSampleMs) >= APP_IMU_STALE_TIMEOUT_MS) {
      s_demoMode = true;
      s_status = APP_IMU_STATUS_DEMO;
      s_demoStartMs = nowMs;
    }
  }
}

const ImuSample_t *AppImu_Latest(void) { return &s_sample; }

AppImuStatus_t AppImu_Status(void) { return s_status; }
