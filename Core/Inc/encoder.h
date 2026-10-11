#ifndef FINISH_ENCODER_H
#define FINISH_ENCODER_H

#include "main.h"

/*
 * 速度单位换算参数：
 * ENCODER_COUNTS_PER_OUTPUT_REV 是电机输出轴转一圈，TIM2 四倍频后的计数增量。
 * 默认 1320 = 11 PPR * 4 * 30 减速比。不同电机请用实际值替换。
 */
#ifndef ENCODER_COUNTS_PER_OUTPUT_REV
#define ENCODER_COUNTS_PER_OUTPUT_REV 1320.0f
#endif

/*
 * 编码器计数方向相对电机驱动方向的极性（速度环反馈符号）。
 * 判据：手动正转输出轴时，Encoder_GetSpeed() 应为正；反转为负。
 * 若速度环输出始终饱和、且给定正负都朝同一方向转（转速不跟随给定），
 * 说明反馈极性相反，应把它取反（等价于对调编码器 A/B 两根线）。
 */
#ifndef ENCODER_DIRECTION_SIGN
#define ENCODER_DIRECTION_SIGN -1.0f
#endif

#ifndef ENCODER_SPEED_LPF_ALPHA
#define ENCODER_SPEED_LPF_ALPHA 0.35f
#endif

void Encoder_Init(void);
int16_t Encoder_GetCount(void);
float Encoder_GetSpeed(uint16_t period_ms);
float Encoder_GetSpeedRpm(uint16_t period_ms);
void Encoder_Reset(void);

#endif // FINISH_ENCODER_H
