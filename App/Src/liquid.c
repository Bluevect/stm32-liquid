/**
 * @file liquid.c
 * @brief 重力驱动的液体模拟：列高度场 + 静水压水面 + 平滑晃动 + 抖动水面。
 *
 * 为什么用列高度场而不是二维质量网格
 * ----------------------------------
 * 早期版本用 32 x 16 的单元质量网格，每格最多一个单位液体。整块区域被填满后，
 * 相邻的两个满格之间没有空位，液体无法横向重新分布，板子倾斜时水面根本抬不
 * 起来。列模型没有这个约束：每列直接改变高度，质量守恒也只是一次求和。
 */

#include <math.h>
#include <stdint.h>

#include "liquid.h"

/* ========================================================================== */
/* 可调参数                                                                   */
/* ========================================================================== */

/** 每个 20 ms 步长向目标剖面靠近的比例（越小越黏、晃动越慢）。 */
#define LIQUID_RELAX_RATE 0.18f
/** 每个 20 ms 步长的波纹衰减（0 = 立即消失，1 = 永不衰减）。 */
#define LIQUID_RIPPLE_DECAY 0.90f
/** 水面角度每变化 1，注入的波纹能量。 */
#define LIQUID_RIPPLE_GAIN 1.20f
/** 波纹最大幅度，单位：格（1 格 = 4 px）。 */
#define LIQUID_RIPPLE_MAX 0.55f
/** 波纹的空间频率，单位：弧度/列。 */
#define LIQUID_RIPPLE_FREQ 0.62f
/** 重力幅度低于该值时视为“没有面内重力”。 */
#define LIQUID_G_EPS 0.02f
/** 计算水面斜率时 |gy| 的下限，避免除零。 */
#define LIQUID_GY_EPS 0.03f
/** 上述速率标定对应的名义步长。 */
#define LIQUID_DT_NOMINAL 0.02f
#define LIQUID_DT_MIN 0.001f
#define LIQUID_DT_MAX 0.05f
/** 演示用的初始液面高度，单位：格。 */
#define LIQUID_DEMO_HEIGHT 7.2f

/* ========================================================================== */
/* 模块状态                                                                   */
/* ========================================================================== */

/** 每列液面高度，单位：格，0 .. LIQUID_GRID_H。 */
static float s_h[LIQUID_GRID_W];
/** 水面波纹的幅度与相位。 */
static float s_rippleAmp;
static float s_ripplePhase;
/** 上一次更新时的水面斜率（gx / |gy|），用于检测角度变化并注入波纹。 */
static float s_surfaceAngle;
/** 屏幕坐标下的重力，各分量在 [-1, 1]。 */
static float s_gx;
static float s_gy;
/** 非零表示“下”是屏幕上方（板子倒置）。 */
static uint8_t s_mirror;

/* ========================================================================== */
/* 内部函数                                                                   */
/* ========================================================================== */

/**
 * @brief 把单列液面高度限制到屏幕范围内。
 * @param[in] h 高度，单位：格。
 * @return 限幅后的高度。
 */
static float liquidClampHeight(float h) {
  if (!(h >= 0.0f)) { /* 负数或 NaN */
    return 0.0f;
  }
  if (h > (float)LIQUID_GRID_H) {
    return (float)LIQUID_GRID_H;
  }
  return h;
}

/**
 * @brief 强制一列高度剖面之和等于容器里的液体总量。
 *
 * 裁剪后的水面不一定能精确表示任意液体量（例如板子横滚 90 度时斜率比屏幕高度还
 * 陡，剖面会在两列之间从 0 跳到 H）。这里把残差补到水面附近、还有空间的最高列
 * 上，液体因此保持成一团，而不是在整块屏幕上摊成一层薄膜。
 *
 * @param[in,out] h 高度剖面，单位：格。
 * @param[in] total 剖面应有的总和。
 */
static void liquidSetTotal(float *h, float total) {
  for (uint32_t it = 0U; it < 8U; ++it) {
    float sum = 0.0f;
    float diff;
    float bestH = -1.0f;
    uint32_t best = LIQUID_GRID_W;

    for (uint32_t x = 0U; x < LIQUID_GRID_W; ++x) {
      sum += h[x];
    }
    diff = total - sum;
    if (fabsf(diff) < 1e-4f) {
      return;
    }

    /* 选“还装得下、并且当前最高”的列，保持液体紧凑。 */
    for (uint32_t x = 0U; x < LIQUID_GRID_W; ++x) {
      const float room = (diff > 0.0f) ? ((float)LIQUID_GRID_H - h[x]) : h[x];

      if ((room > 1e-6f) && (h[x] > bestH)) {
        bestH = h[x];
        best = x;
      }
    }
    if (best == LIQUID_GRID_W) {
      return;
    }

    if (diff > 0.0f) {
      const float room = (float)LIQUID_GRID_H - h[best];

      h[best] += (diff < room) ? diff : room;
    } else {
      h[best] -= ((-diff) < h[best]) ? (-diff) : h[best];
    }
    h[best] = liquidClampHeight(h[best]);
  }
}

/**
 * @brief 按当前重力构造静水压目标剖面（质量守恒）。
 * @param[out] target 每列目标高度，单位：格。
 * @param[in] total 液体总量，单位：格·列。
 */
static void liquidBuildTarget(float *target, float total) {
  const float gy = (fabsf(s_gy) < LIQUID_GY_EPS) ? LIQUID_GY_EPS : fabsf(s_gy);
  const float slope = s_gx / gy;
  /* |gy| 很小时（板子接近竖直）斜率极大，搜索区间必须足够宽。 */
  const float span = fabsf(slope) * (float)LIQUID_GRID_W + (float)LIQUID_GRID_H;
  float lo = -span;
  float hi = span;
  float h0;
  float sum = 0.0f;

  /* 裁剪后的剖面总和随 h0 单调递增，二分即可反解质量守恒的偏移。 */
  for (uint32_t it = 0U; it < 24U; ++it) {
    const float mid = 0.5f * (lo + hi);

    sum = 0.0f;
    for (uint32_t x = 0U; x < LIQUID_GRID_W; ++x) {
      sum += liquidClampHeight(mid + slope * (float)x);
    }
    if (sum > total) {
      hi = mid;
    } else {
      lo = mid;
    }
  }

  h0 = 0.5f * (lo + hi);
  for (uint32_t x = 0U; x < LIQUID_GRID_W; ++x) {
    target[x] = liquidClampHeight(h0 + slope * (float)x);
  }
  liquidSetTotal(target, total);
}

/**
 * @brief 推进波纹并输出每列的高度偏移（均值约为 0）。
 * @param[in] dtScale 当前步长/名义步长。
 * @param[out] offset 每列高度偏移，单位：格。
 */
static void liquidRipple(float dtScale, float *offset) {
  float mean = 0.0f;

  for (uint32_t x = 0U; x < LIQUID_GRID_W; ++x) {
    offset[x] = s_rippleAmp * sinf(LIQUID_RIPPLE_FREQ * (float)x + s_ripplePhase);
    mean += offset[x];
  }
  mean /= (float)LIQUID_GRID_W;
  for (uint32_t x = 0U; x < LIQUID_GRID_W; ++x) {
    offset[x] -= mean;
  }

  s_ripplePhase += 0.9f * dtScale;
  if (s_ripplePhase > 6.2831853f) {
    s_ripplePhase -= 6.2831853f;
  }
  s_rippleAmp *= powf(LIQUID_RIPPLE_DECAY, dtScale);
}

/* ========================================================================== */
/* 公共接口                                                                   */
/* ========================================================================== */

void Liquid_Init(void) {
  for (uint32_t x = 0U; x < LIQUID_GRID_W; ++x) {
    s_h[x] = 0.0f;
  }
  s_rippleAmp = 0.0f;
  s_ripplePhase = 0.0f;
  s_surfaceAngle = 0.0f;
  s_gx = 0.0f;
  s_gy = 0.0f;
  s_mirror = 0U;
}

void Liquid_SetGravity(float gx, float gy) {
  /* 限制在单位正方形内；NaN 当作失重处理。 */
  if (!(gx == gx)) {
    gx = 0.0f;
  }
  if (!(gy == gy)) {
    gy = 0.0f;
  }
  if (gx > 1.0f) {
    gx = 1.0f;
  } else if (gx < -1.0f) {
    gx = -1.0f;
  }
  if (gy > 1.0f) {
    gy = 1.0f;
  } else if (gy < -1.0f) {
    gy = -1.0f;
  }
  s_gx = gx;
  s_gy = gy;
}

void Liquid_Update(float dt_s) {
  float target[LIQUID_GRID_W];
  float ripple[LIQUID_GRID_W];
  float total = 0.0f;
  float scale;
  float relax;
  float gmag;
  float angle;

  if (!(dt_s == dt_s)) { /* NaN 保护 */
    return;
  }
  if (dt_s < LIQUID_DT_MIN) {
    dt_s = LIQUID_DT_MIN;
  } else if (dt_s > LIQUID_DT_MAX) {
    dt_s = LIQUID_DT_MAX;
  }
  scale = dt_s / LIQUID_DT_NOMINAL;

  /* “下”默认是屏幕底部，板子倒置时镜像。 */
  s_mirror = (s_gy < 0.0f) ? 1U : 0U;

  gmag = sqrtf(s_gx * s_gx + s_gy * s_gy);
  if (gmag < LIQUID_G_EPS) {
    /*
     * 没有面内重力（板子平放）：真实液体没有可以流的方向，水面冻结，只让已有
     * 波纹慢慢衰减。
     */
    liquidRipple(scale, ripple);
    return;
  }

  for (uint32_t x = 0U; x < LIQUID_GRID_W; ++x) {
    total += s_h[x];
  }

  /* --- 水面角度变化 -> 注入波纹能量 -------------------------------------- */
  angle = s_gx / ((fabsf(s_gy) < LIQUID_GY_EPS) ? LIQUID_GY_EPS : fabsf(s_gy));
  s_rippleAmp += LIQUID_RIPPLE_GAIN * fabsf(angle - s_surfaceAngle) * 0.05f;
  if (s_rippleAmp > LIQUID_RIPPLE_MAX) {
    s_rippleAmp = LIQUID_RIPPLE_MAX;
  }
  s_surfaceAngle = angle;

  /* --- 向静水压目标剖面弛豫 ---------------------------------------------- */
  liquidBuildTarget(target, total);
  liquidRipple(scale, ripple);
  relax = 1.0f - powf(1.0f - LIQUID_RELAX_RATE, scale);

  for (uint32_t x = 0U; x < LIQUID_GRID_W; ++x) {
    const float goal = liquidClampHeight(target[x] + ripple[x]);

    s_h[x] += relax * (goal - s_h[x]); /* 一阶滞后：不会积累动量 */
    s_h[x] = liquidClampHeight(s_h[x]);
  }

  /* 把一阶插值留下的微小残差清掉，保证长时间运行质量不漂移。 */
  liquidSetTotal(s_h, total);
}

void Liquid_Render(BSP_OLED_t *display) {
  static const uint8_t bayer[2][2] = {{0U, 2U}, {3U, 1U}};

  if (display == NULL) {
    return;
  }

  for (int32_t x = 0; x < (int32_t)BSP_OLED_WIDTH; ++x) {
    const int32_t cell = (x / (int32_t)LIQUID_CELL_PX < (int32_t)LIQUID_GRID_W - 1)
                             ? (x / (int32_t)LIQUID_CELL_PX)
                             : ((int32_t)LIQUID_GRID_W - 2);
    const float frac = (float)(x % (int32_t)LIQUID_CELL_PX) / (float)LIQUID_CELL_PX;
    const float h = s_h[cell] + (s_h[cell + 1] - s_h[cell]) * frac;
    const float hPx = h * (float)LIQUID_CELL_PX;

    for (int32_t y = 0; y < (int32_t)BSP_OLED_HEIGHT; ++y) {
      /* 该像素到容器“底”的距离。 */
      const int32_t row = s_mirror ? y : ((int32_t)BSP_OLED_HEIGHT - 1 - y);
      const float level = hPx - (float)row;

      if (level >= 1.0f) {
        BSP_OLED_DrawPixel(display, x, y, true); /* 实心液体 */
      } else if (level > 0.0f) {
        /* 2x2 有序抖动的水面抗锯齿。 */
        const float thr = (float)bayer[x & 1][y & 1] * 0.25f;

        if (level > thr) {
          BSP_OLED_DrawPixel(display, x, y, true);
        }
      }
    }
  }
}

void Liquid_ResetDemoFill(void) {
  for (uint32_t x = 0U; x < LIQUID_GRID_W; ++x) {
    s_h[x] = LIQUID_DEMO_HEIGHT;
  }
  s_rippleAmp = 0.0f;
  s_ripplePhase = 0.0f;
  s_surfaceAngle = 0.0f;
  s_mirror = 0U;
}
