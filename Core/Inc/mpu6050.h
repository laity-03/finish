//
// Created by Laity on 2026/10/3.
//

#ifndef FINISH_MPU6050_H
#define FINISH_MPU6050_H
//
// MPU6050 底层驱动（I2C1：SCL=PB6，SDA=PB7）
//

#pragma once

#include <stdint.h>

/** MPU6050 数据（已换算为物理量） */
typedef struct {
    float accel[3];  /**< 加速度，单位 g */
    float gyro[3];   /**< 角速度，单位 °/s（已去除零偏） */
    float temp;      /**< 温度，单位 ℃ */
    uint8_t valid;   /**< 1 = 本次读取成功；0 = 读取失败，其余字段无效 */
} bsp_mpu6050_data_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 探测并初始化 MPU6050
 * @note  会自动尝试 AD0=0(0x68) 和 AD0=1(0x69) 两个地址，用 WHO_AM_I 确认器件在线。
 * @return 1 = 找到器件并配置成功；0 = 总线无响应（检查供电和 SCL/SDA 接线）
 */
uint8_t bsp_mpu6050_init(void);

/** @return 1 = 器件在线；0 = 不在线 */
uint8_t bsp_mpu6050_is_ready(void);

/** @return 探测到的 7 位从机地址（0x68 / 0x69） */
uint8_t bsp_mpu6050_address(void);

/**
 * @brief 读取 MPU6050 数据
 * @return 换算后的加速度、角速度与温度；valid 字段指示本次是否读取成功
 */
bsp_mpu6050_data_t bsp_mpu6050_read(void);

/**
 * @brief 陀螺仪零偏校准，调用期间请保持静止
 * @param samples 采样次数
 */
void bsp_mpu6050_calibrate(uint16_t samples);

#ifdef __cplusplus
}
#endif

#endif //FINISH_MPU6050_H