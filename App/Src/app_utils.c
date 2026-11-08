/**
 * @file app_utils.c
 * @brief 应用层通用工具实现（裸机周期调度）。
 */

#include <stddef.h>

#include "app_utils.h"

void AppUtils_PeriodicInit(AppUtils_Periodic_t *timer, uint32_t nowMs, uint32_t periodMs) {
  if (timer == NULL) {
    return;
  }

  timer->periodMs = (periodMs == 0U) ? 1U : periodMs;
  timer->nextMs = nowMs + timer->periodMs;
}

bool AppUtils_PeriodicDue(AppUtils_Periodic_t *timer, uint32_t nowMs) {
  if (timer == NULL) {
    return false;
  }

  /* 有符号差值对 32 位回绕安全：now - next >= 0 表示已到期。 */
  if ((int32_t)(nowMs - timer->nextMs) < 0) {
    return false;
  }

  timer->nextMs += timer->periodMs;
  /* 如果任务拖得太久，跳过已经错过的周期，避免补跑风暴。 */
  if ((int32_t)(nowMs - timer->nextMs) >= 0) {
    timer->nextMs = nowMs + timer->periodMs;
  }
  return true;
}

uint32_t AppUtils_ElapsedMs(uint32_t nowMs, uint32_t thenMs) { return (uint32_t)(nowMs - thenMs); }
