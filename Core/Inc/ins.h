//
// Created by Laity on 2026/10/3.
//

#ifndef FINISH_INS_H
#define FINISH_INS_H

//
// INS 姿态解算（基于 MPU6050，Mahony 互补滤波）
//

#pragma once

#include <stdint.h>

#include "mpu6050.h"

/** 姿态解算输出 */
typedef struct {
    float q[4];              /**< 姿态四元数 [w, x, y, z] */
    float roll, pitch, yaw;  /**< 姿态角，单位 ° */
    float yaw_total_angle;   /**< 累计偏航角，单位 °，可超过 ±180 */
    float gyro[3];           /**< 角速度，单位 °/s */
    float gyro_accel[3];     /**< 角加速度，单位 °/s² */
    uint32_t timestamp;      /**< 更新时间戳，单位 ms */
    bsp_mpu6050_data_t raw;  /**< 本次读取的底层数据 */
    uint8_t sensor_ok;       /**< 1 = MPU6050 正常返回数据；0 = 未接好/读取失败 */
    uint32_t read_errors;    /**< 累计读取失败次数，用于诊断 I2C 接触问题 */
} ins_data_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 初始化姿态解算（会自动完成陀螺仪零偏校准，期间请保持静止）
 */
void ins_init(void);

/**
 * @brief 执行一次姿态解算，需要周期性调用
 */
void ins_update(void);

/**
 * @brief 获取解算结果
 */
ins_data_t *ins_data(void);

/**
 * @brief 获取角速度数组（°/s），长度为 3
 */
const float *ins_get_gyro(void);

/**
 * @brief 获取角加速度数组（°/s²），长度为 3
 */
const float *ins_get_gyro_accel(void);

#ifdef __cplusplus
}
#endif


#endif //FINISH_INS_H
