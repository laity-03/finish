#ifndef FINISH_PID_H
#define FINISH_PID_H

typedef struct {
    float Kp;               // 比例系数
    float Ki;               // 积分系数
    float Kd;               // 微分系数
    float target;           // 目标值
    float actual;           // 实际值
    float err;              // 当前误差
    float err_last;         // 上次误差
    float actual_last;      // 上次实际值，用于微分先行
    float integral;         // 积分累积
    float derivative;       // 低通后的实际值微分
    float output;           // 输出值
    float output_max;       // 输出限幅
    float integral_max;     // 积分限幅
    float derivative_lpf;   // 微分低通系数
} PID_t;

void PID_Init(PID_t *pid, float kp, float ki, float kd,
              float out_max, float integral_max, float derivative_lpf);
void PID_Reset(PID_t *pid, float actual);
float PID_Calc(PID_t *pid, float target, float actual, float dt);

#endif
