//
// Created by Laity on 2026/10/3.
//
//
// MPU6050 底层驱动（I2C1：SCL=PB6，SDA=PB7）
//

#include "mpu6050.h"

#include "i2c.h"

// 寄存器地址
#define MPU6050_REG_SMPLRT_DIV   0x19
#define MPU6050_REG_CONFIG       0x1A
#define MPU6050_REG_GYRO_CONFIG  0x1B
#define MPU6050_REG_ACCEL_CONFIG 0x1C
#define MPU6050_REG_ACCEL_XOUT_H 0x3B
#define MPU6050_REG_GYRO_XOUT_H  0x43
#define MPU6050_REG_PWR_MGMT_1   0x6B
#define MPU6050_REG_WHO_AM_I     0x75

#define MPU6050_ADDR_LOW   ((uint16_t)(0x68U << 1))  // AD0 接地
#define MPU6050_ADDR_HIGH  ((uint16_t)(0x69U << 1))  // AD0 接高电平
#define MPU6050_TIMEOUT    100U                      // I2C 超时时间，单位 ms

// 满量程对应的灵敏度
#define MPU6050_ACCEL_SEN 16384.0f  // ±2g：      LSB/g
#define MPU6050_GYRO_SEN  131.0f    // ±250°/s：  LSB/(°/s)

static uint16_t mpu_addr = MPU6050_ADDR_LOW;  // 当前使用的 8 位从机地址
static uint8_t  mpu_online = 0U;              // 1 = 已探测到器件
static float gyro_bias[3];                    // 陀螺仪零偏，单位 °/s

/**
 * @brief 写单个寄存器
 */
static uint8_t write_reg(uint8_t reg, uint8_t val) {
    return HAL_I2C_Mem_Write(&hi2c1, mpu_addr, reg, I2C_MEMADD_SIZE_8BIT,
                             &val, 1, MPU6050_TIMEOUT) == HAL_OK;
}

/**
 * @brief 连续读寄存器
 */
static uint8_t read_reg(uint8_t reg, uint8_t *buf, uint16_t len) {
    return HAL_I2C_Mem_Read(&hi2c1, mpu_addr, reg, I2C_MEMADD_SIZE_8BIT,
                            buf, len, MPU6050_TIMEOUT) == HAL_OK;
}

/**
 * @brief 用 WHO_AM_I 判断指定地址上是否挂着 MPU6050
 */
static uint8_t probe_addr(uint16_t addr) {
    uint8_t who = 0U;

    mpu_addr = addr;
    if (!read_reg(MPU6050_REG_WHO_AM_I, &who, 1U)) {
        return 0U;   // 无应答
    }

    /* MPU6050 固定返回 0x68；兼容部分国产替代返回 0x70/0x71。
     * 0x00 / 0xFF 视为总线异常，不接受。 */
    if (who == 0x00U || who == 0xFFU) {
        return 0U;
    }

    return 1U;
}

uint8_t bsp_mpu6050_init(void) {
    mpu_online = 0U;

    HAL_Delay(50);

    /* 先探测器件：AD0 接法不同会导致地址是 0x68 或 0x69 */
    if (!probe_addr(MPU6050_ADDR_LOW) && !probe_addr(MPU6050_ADDR_HIGH)) {
        return 0U;
    }

    write_reg(MPU6050_REG_PWR_MGMT_1, 0x80);   // 复位
    HAL_Delay(100);
    write_reg(MPU6050_REG_PWR_MGMT_1, 0x01);   // 唤醒，时钟源选择 X 轴陀螺 PLL
    HAL_Delay(10);

    write_reg(MPU6050_REG_CONFIG, 0x03);        // 数字低通滤波 44Hz
    write_reg(MPU6050_REG_SMPLRT_DIV, 0x09);    // 采样率 = 1kHz / (1 + 9) = 100Hz
    write_reg(MPU6050_REG_GYRO_CONFIG, 0x00);   // 陀螺仪 ±250°/s
    write_reg(MPU6050_REG_ACCEL_CONFIG, 0x00);  // 加速度计 ±2g

    mpu_online = 1U;
    return 1U;
}

uint8_t bsp_mpu6050_is_ready(void) {
    return mpu_online;
}

uint8_t bsp_mpu6050_address(void) {
    return (uint8_t)(mpu_addr >> 1);
}

bsp_mpu6050_data_t bsp_mpu6050_read(void) {
    bsp_mpu6050_data_t ret = {0};
    uint8_t buf[14];

    // valid 默认为 0：读取失败时调用方不会用到这些零值
    if (!mpu_online) {
        return ret;
    }

    // 读取：加速度(6) + 温度(2) + 角速度(6)
    if (read_reg(MPU6050_REG_ACCEL_XOUT_H, buf, sizeof(buf))) {
        int16_t ax = (int16_t)((buf[0] << 8) | buf[1]);
        int16_t ay = (int16_t)((buf[2] << 8) | buf[3]);
        int16_t az = (int16_t)((buf[4] << 8) | buf[5]);
        int16_t temp = (int16_t)((buf[6] << 8) | buf[7]);
        int16_t gx = (int16_t)((buf[8] << 8) | buf[9]);
        int16_t gy = (int16_t)((buf[10] << 8) | buf[11]);
        int16_t gz = (int16_t)((buf[12] << 8) | buf[13]);

        ret.accel[0] = (float)ax / MPU6050_ACCEL_SEN;
        ret.accel[1] = (float)ay / MPU6050_ACCEL_SEN;
        ret.accel[2] = (float)az / MPU6050_ACCEL_SEN;

        ret.gyro[0] = (float)gx / MPU6050_GYRO_SEN - gyro_bias[0];
        ret.gyro[1] = (float)gy / MPU6050_GYRO_SEN - gyro_bias[1];
        ret.gyro[2] = (float)gz / MPU6050_GYRO_SEN - gyro_bias[2];

        ret.temp = (float)temp / 340.0f + 36.53f;
        ret.valid = 1U;
    }

    return ret;
}

void bsp_mpu6050_calibrate(uint16_t samples) {
    if (samples == 0U || !mpu_online) {
        return;
    }

    uint8_t buf[6];
    float sum[3] = {0};
    uint16_t ok_count = 0U;

    for (uint16_t i = 0; i < samples; i++) {
        if (read_reg(MPU6050_REG_GYRO_XOUT_H, buf, sizeof(buf))) {
            sum[0] += (float)(int16_t)((buf[0] << 8) | buf[1]) / MPU6050_GYRO_SEN;
            sum[1] += (float)(int16_t)((buf[2] << 8) | buf[3]) / MPU6050_GYRO_SEN;
            sum[2] += (float)(int16_t)((buf[4] << 8) | buf[5]) / MPU6050_GYRO_SEN;
            ok_count++;
        }
        HAL_Delay(1);
    }

    if (ok_count > 0U) {
        gyro_bias[0] = sum[0] / (float)ok_count;
        gyro_bias[1] = sum[1] / (float)ok_count;
        gyro_bias[2] = sum[2] / (float)ok_count;
    } else {
        gyro_bias[0] = 0.0f;
        gyro_bias[1] = 0.0f;
        gyro_bias[2] = 0.0f;
    }
}