/**
 * @file bsp_mpu6050_dmp.c
 * @brief MPU6050 DMP 初始化与姿态读取。
 */

#include "bsp_mpu6050.h"

#include <math.h>

#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "inv_mpu_port.h"

#define MPU6050_DMP_ADDRESS 0x68U
#define MPU6050_DMP_SAMPLE_RATE 100U
#define MPU6050_DMP_FIFO_RATE 50U
#define MPU6050_FIFO_COUNT_H 0x72U
#define MPU6050_FIFO_COUNT_L 0x73U
#define MPU6050_DMP_PACKET_LENGTH 16U
#define MPU6050_Q30_SCALE 1073741824.0f
#define MPU6050_RAD_TO_DEG 57.2957795f

static const signed char gyroOrientation[9] = {-1, 0, 0, 0, -1, 0, 0, 0, 1};

/**
 * @brief 转换单个方向向量。
 * @param[in] row 方向向量。
 * @return 编码后的方向值。
 */
static uint16_t invRowToScale(const signed char *row) {
  uint16_t scale;

  if (row[0] > 0) {
    scale = 0U;
  } else if (row[0] < 0) {
    scale = 4U;
  } else if (row[1] > 0) {
    scale = 1U;
  } else if (row[1] < 0) {
    scale = 5U;
  } else if (row[2] > 0) {
    scale = 2U;
  } else if (row[2] < 0) {
    scale = 6U;
  } else {
    scale = 7U;
  }

  return scale;
}

/**
 * @brief 转换安装方向矩阵。
 * @param[in] matrix 3x3 方向矩阵。
 * @return 编码后的方向值。
 */
static uint16_t invOrientationMatrixToScalar(const signed char *matrix) {
  uint16_t scalar = invRowToScale(matrix);

  scalar |= (uint16_t)(invRowToScale(matrix + 3) << 3U);
  scalar |= (uint16_t)(invRowToScale(matrix + 6) << 6U);
  return scalar;
}

/**
 * @brief 将角度转换为百分之一度。
 * @param[in] angleDeg 角度。
 * @return 百分之一度。
 */
static int32_t angleToHundredths(float angleDeg) {
  float scaled = angleDeg * 100.0f;

  return (int32_t)(scaled >= 0.0f ? (scaled + 0.5f) : (scaled - 0.5f));
}

/**
 * @brief 读取 FIFO 字节数。
 * @param[out] fifoCount FIFO 字节数。
 * @return 成功返回 true，否则返回 false。
 */
static bool readFifoCount(uint16_t *fifoCount) {
  uint8_t highByte;
  uint8_t lowByte;

  if (mpu_read_reg(MPU6050_FIFO_COUNT_H, &highByte) != 0 || mpu_read_reg(MPU6050_FIFO_COUNT_L, &lowByte) != 0) {
    return false;
  }

  *fifoCount = (uint16_t)(((uint16_t)highByte << 8U) | lowByte);
  return true;
}

/**
 * @brief 初始化并启动 DMP。
 * @param[out] device MPU6050 实例。
 * @param[in] i2cBus 所属 I2C 总线。
 * @return 成功返回 true，否则返回 false。
 */
bool MPU6050_InitDmp(MPU6050_t *device, I2C_Bus_t *i2cBus) {
  int result = 0;

  if ((device == NULL) || (i2cBus == NULL)) {
    return false;
  }

  if ((I2C_Hardware_Attach(&device->i2c, i2cBus, MPU6050_DMP_ADDRESS) != HAL_OK) ||
      (I2C_Hardware_IsReady(&device->i2c, 3U, 100U) != HAL_OK) || (InvMpuPort_Init(i2cBus, MPU6050_DMP_ADDRESS) != 0)) {
    return false;
  }

  result += mpu_init();
  result += mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL);
  result += mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
  result += mpu_set_sample_rate(MPU6050_DMP_SAMPLE_RATE);
  result += dmp_load_motion_driver_firmware();
  result += dmp_set_orientation(invOrientationMatrixToScalar(gyroOrientation));
  result += dmp_enable_feature(DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_GYRO_CAL);
  result += dmp_set_fifo_rate(MPU6050_DMP_FIFO_RATE);
  result += mpu_set_dmp_state(1);

  if (result != 0) {
    return false;
  }

  return true;
}

/**
 * @brief 读取最新 DMP 姿态数据。
 * @param[in] device MPU6050 实例。
 * @param[out] data 姿态数据。
 * @return DMP 读取状态。
 */
MPU6050_DmpStatus_t MPU6050_ReadDmp(const MPU6050_t *device, MPU6050_DmpData_t *data) {
  uint16_t fifoCount;
  short gyro[3];
  short accel[3];
  long quaternion[4];
  short sensors;
  unsigned char more;
  unsigned long timestamp;

  if ((device == NULL) || (data == NULL)) {
    return MPU6050_DMP_ERROR;
  }

  if (!readFifoCount(&fifoCount)) {
    return MPU6050_DMP_ERROR;
  }

  if (fifoCount < MPU6050_DMP_PACKET_LENGTH) {
    return MPU6050_DMP_NO_DATA;
  }

  do {
    const int result = dmp_read_fifo(gyro, accel, quaternion, &timestamp, &sensors, &more);
    if (result != 0) {
      return MPU6050_DMP_ERROR;
    }
  } while (more != 0U);

  if ((sensors & INV_WXYZ_QUAT) == 0) {
    return MPU6050_DMP_NO_DATA;
  }

  float q0 = (float)quaternion[0] / MPU6050_Q30_SCALE;
  float q1 = (float)quaternion[1] / MPU6050_Q30_SCALE;
  float q2 = (float)quaternion[2] / MPU6050_Q30_SCALE;
  float q3 = (float)quaternion[3] / MPU6050_Q30_SCALE;
  float sinPitch = -2.0f * q1 * q3 + 2.0f * q0 * q2;

  if (sinPitch > 1.0f) {
    sinPitch = 1.0f;
  } else if (sinPitch < -1.0f) {
    sinPitch = -1.0f;
  }

  data->quaternionQ30[0] = quaternion[0];
  data->quaternionQ30[1] = quaternion[1];
  data->quaternionQ30[2] = quaternion[2];
  data->quaternionQ30[3] = quaternion[3];
  data->pitchDegX100 = angleToHundredths(asinf(sinPitch) * MPU6050_RAD_TO_DEG);
  data->rollDegX100 = angleToHundredths(
      atan2f(2.0f * q2 * q3 + 2.0f * q0 * q1, -2.0f * q1 * q1 - 2.0f * q2 * q2 + 1.0f) * MPU6050_RAD_TO_DEG);
  data->yawDegX100 =
      angleToHundredths(atan2f(2.0f * (q1 * q2 + q0 * q3), q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3) * MPU6050_RAD_TO_DEG);
  data->timestampMs = timestamp;
  data->valid = true;

  /*
   * 板体系重力方向：世界系重力 (0, 0, -1) 经四元数旋转回板体系，
   * 即 -R(q) 的第三行。与欧拉角不同，这个表达式在任意姿态都没有奇异点。
   */
  data->gravityX = 2.0f * (q0 * q2 - q1 * q3);
  data->gravityY = -2.0f * (q0 * q1 + q2 * q3);
  data->gravityZ = 2.0f * (q1 * q1 + q2 * q2) - 1.0f;

  return MPU6050_DMP_DATA_READY;
}
