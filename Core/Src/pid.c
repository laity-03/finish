#include "pid.h"

static float clampf(float value, float min_value, float max_value)
{
    if (value > max_value) return max_value;
    if (value < min_value) return min_value;
    return value;
}

void PID_Init(PID_t *pid, float kp, float ki, float kd,
              float out_max, float integral_max, float derivative_lpf)
{
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;
    pid->output_max = out_max;
    pid->integral_max = integral_max;
    pid->derivative_lpf = derivative_lpf;
    PID_Reset(pid, 0.0f);
}

void PID_Reset(PID_t *pid, float actual)
{
    pid->target = actual;
    pid->actual = actual;
    pid->err = 0.0f;
    pid->err_last = 0.0f;
    pid->actual_last = actual;
    pid->integral = 0.0f;
    pid->derivative = 0.0f;
    pid->output = 0.0f;
}

float PID_Calc(PID_t *pid, float target, float actual, float dt)
{
    if (dt <= 0.0f) {
        dt = 0.01f;
    }

    pid->target = target;
    pid->actual = actual;
    pid->err = pid->target - pid->actual;

    // 微分先行：对实际值求导，避免目标值突变时产生微分冲击。
    float derivative_raw = (pid->actual - pid->actual_last) / dt;
    pid->actual_last = pid->actual;
    pid->derivative += pid->derivative_lpf * (derivative_raw - pid->derivative);

    float integral_candidate = pid->integral + pid->err * dt;
    integral_candidate = clampf(integral_candidate,
                                -pid->integral_max,
                                pid->integral_max);

    float output_candidate = pid->Kp * pid->err
                           + pid->Ki * integral_candidate
                           - pid->Kd * pid->derivative;

    // 输出饱和时停止同方向积分，避免积分饱和。
    if ((output_candidate > pid->output_max && pid->err > 0.0f) ||
        (output_candidate < -pid->output_max && pid->err < 0.0f)) {
        output_candidate = pid->Kp * pid->err
                         + pid->Ki * pid->integral
                         - pid->Kd * pid->derivative;
    } else {
        pid->integral = integral_candidate;
    }

    pid->output = clampf(output_candidate,
                         -pid->output_max,
                         pid->output_max);
    pid->err_last = pid->err;

    return pid->output;
}
