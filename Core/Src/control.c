#include "control.h"

#include <math.h>
#include <string.h>

#include "encoder.h"
#include "ins.h"
#include "key.h"
#include "knob.h"
#include "motor.h"
#include "pid.h"

static PID_t pid_angle;
static PID_t pid_speed;

static ControlMode_t control_mode = CONTROL_MODE_ANGLE;
static float target_angle_deg = 0.0f;
static float target_speed_rpm = 0.0f;
static float angle_zero_offset_deg = 0.0f;

static float actual_angle_deg = 0.0f;
static float actual_speed_rpm = 0.0f;
static float actual_speed_counts = 0.0f;
static int16_t encoder_count = 0;
static float speed_target_rpm = 0.0f;
static float pwm_output = 0.0f;

static const float speed_presets_rpm[] = {
    0.0f, 30.0f, 60.0f, 100.0f, -30.0f, -60.0f, -100.0f
};
static uint8_t speed_preset_index = 0U;

/* 角度按键档位：0 -> 10 -> 30 -> 60 -> 0 ... */
static const float angle_presets_deg[] = {
    0.0f, 10.0f, 30.0f, 60.0f
};
static uint8_t angle_preset_index = 0U;

static Key_t key_speed;
static Key_t key_angle;

static float knob_command_deg = 0.0f;
static uint8_t knob_target_initialized = 0U;

static float clampf(float value, float min_value, float max_value)
{
    if (value > max_value) return max_value;
    if (value < min_value) return min_value;
    return value;
}

static float Wrap180(float angle)
{
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

static float Control_GetRawAngle(void)
{
#if CONTROL_ANGLE_USE_ROLL
    return ins_data()->roll;
#else
    return ins_data()->pitch;
#endif
}

static float Control_KnobPositionToAngle(float position)
{
    float angle = (position * 2.0f - 1.0f) * CONTROL_TARGET_ANGLE_MAX_DEG;
    return clampf(angle, -CONTROL_TARGET_ANGLE_MAX_DEG,
                  CONTROL_TARGET_ANGLE_MAX_DEG);
}

static void Control_ApplyKnobTarget(float dt_s)
{
    if (!Knob_IsReady()) {
        return;
    }

    float knob_target_deg =
        Control_KnobPositionToAngle(Knob_GetPosition());

    if (!knob_target_initialized) {
        knob_command_deg = knob_target_deg;
        target_angle_deg = knob_command_deg;
        knob_target_initialized = 1U;
        return;
    }

    float max_step = CONTROL_KNOB_SLEW_DEG_PER_S * dt_s;
    float delta = clampf(knob_target_deg - knob_command_deg,
                         -max_step, max_step);
    if (fabsf(delta) < CONTROL_KNOB_DEADBAND_DEG) {
        return;
    }

    uint8_t mode_changed = (control_mode != CONTROL_MODE_ANGLE);
    knob_command_deg += delta;
    target_angle_deg = knob_command_deg;
    target_speed_rpm = 0.0f;
    control_mode = CONTROL_MODE_ANGLE;

    if (mode_changed) {
        PID_Reset(&pid_speed, actual_speed_rpm);
        PID_Reset(&pid_angle, actual_angle_deg);
    }
}

static void Control_ApplyKeyEvents(KeyEvent_t speed_event, KeyEvent_t angle_event)
{
    if (speed_event == KEY_EVENT_SHORT_PRESS) {
        control_mode = CONTROL_MODE_SPEED;
        speed_preset_index = (uint8_t)((speed_preset_index + 1U) %
                                       (sizeof(speed_presets_rpm) / sizeof(speed_presets_rpm[0])));
        target_speed_rpm = speed_presets_rpm[speed_preset_index];
        PID_Reset(&pid_speed, actual_speed_rpm);
        PID_Reset(&pid_angle, actual_angle_deg);
    } else if (speed_event == KEY_EVENT_LONG_PRESS) {
        control_mode = CONTROL_MODE_ANGLE;
        target_speed_rpm = 0.0f;
        PID_Reset(&pid_speed, actual_speed_rpm);
        PID_Reset(&pid_angle, actual_angle_deg);
    }

    if (angle_event == KEY_EVENT_SHORT_PRESS) {
        angle_preset_index = (uint8_t)((angle_preset_index + 1U) %
                                       (sizeof(angle_presets_deg) /
                                        sizeof(angle_presets_deg[0])));
        Control_SetTargetAngle(angle_presets_deg[angle_preset_index]);
    } else if (angle_event == KEY_EVENT_LONG_PRESS) {
        angle_preset_index = 0U;
        Control_SetTargetAngle(0.0f);
    }
}

void Control_Init(void)
{
    Key_Init(&key_speed, KEY_SPEED_GPIO_Port, KEY_SPEED_Pin);
    Key_Init(&key_angle, KEY_ANGLE_GPIO_Port, KEY_ANGLE_Pin);
    Knob_Init();
    Motor_Stop();
    Encoder_Reset();

    PID_Init(&pid_angle, 2.5f, 0.50f, 0.08f, 80.0f, 80.0f, 0.20f);
    PID_Init(&pid_speed, 1.0f, 1.5f, 0.0f, 100.0f, 100.0f, 0.10f);

    control_mode = CONTROL_MODE_ANGLE;
    target_angle_deg = 0.0f;
    target_speed_rpm = 0.0f;
    speed_preset_index = 0U;
    angle_preset_index = 0U;

    /*
     * 上电时请把雪糕棒保持在你希望定义为 0 度的位置。
     * 这段时间 MPU6050 继续更新，最后记录当前姿态为零点。
     */
    for (uint32_t i = 0U; i < CONTROL_ZERO_CALIBRATION_SAMPLES; i++) {
        ins_update();
        HAL_Delay(CONTROL_PERIOD_MS);
    }

    Control_CaptureAngleZero();
    Knob_Update();
    Control_ApplyKnobTarget(CONTROL_PERIOD_S);
    actual_angle_deg = 0.0f;
    actual_speed_rpm = 0.0f;
    speed_target_rpm = 0.0f;
    pwm_output = 0.0f;

    PID_Reset(&pid_angle, actual_angle_deg);
    PID_Reset(&pid_speed, actual_speed_rpm);
}

void Control_SetMode(ControlMode_t mode)
{
    if (mode == CONTROL_MODE_SPEED) {
        control_mode = CONTROL_MODE_SPEED;
        PID_Reset(&pid_speed, actual_speed_rpm);
    } else {
        control_mode = CONTROL_MODE_ANGLE;
        target_speed_rpm = 0.0f;
        PID_Reset(&pid_angle, actual_angle_deg);
        PID_Reset(&pid_speed, actual_speed_rpm);
    }
}

ControlMode_t Control_GetMode(void)
{
    return control_mode;
}

void Control_SetTargetAngle(float angle_deg)
{
    target_angle_deg = clampf(angle_deg,
                              -CONTROL_TARGET_ANGLE_MAX_DEG,
                              CONTROL_TARGET_ANGLE_MAX_DEG);
    Control_SetMode(CONTROL_MODE_ANGLE);
}

void Control_SetTargetSpeedRpm(float speed_rpm)
{
    target_speed_rpm = clampf(speed_rpm,
                              -CONTROL_TARGET_SPEED_MAX_RPM,
                              CONTROL_TARGET_SPEED_MAX_RPM);
    Control_SetMode(CONTROL_MODE_SPEED);
}

float Control_GetTargetAngle(void)
{
    return target_angle_deg;
}

float Control_GetTargetSpeedRpm(void)
{
    return target_speed_rpm;
}

void Control_CaptureAngleZero(void)
{
    angle_zero_offset_deg = Control_GetRawAngle();
    actual_angle_deg = 0.0f;
    target_angle_deg = 0.0f;
    angle_preset_index = 0U;
    speed_target_rpm = 0.0f;
    pwm_output = 0.0f;
    PID_Reset(&pid_angle, actual_angle_deg);
    PID_Reset(&pid_speed, actual_speed_rpm);
}

void Control_Update(float dt_s)
{
    if (dt_s <= 0.0f) {
        dt_s = CONTROL_PERIOD_S;
    }

    KeyEvent_t speed_event = Key_Update(&key_speed);
    KeyEvent_t angle_event = Key_Update(&key_angle);
    Control_ApplyKeyEvents(speed_event, angle_event);
    Knob_Update();
    Control_ApplyKnobTarget(dt_s);

    ins_update();
    actual_angle_deg = Wrap180(CONTROL_ANGLE_SIGN *
                              (Control_GetRawAngle() - angle_zero_offset_deg));
    uint16_t encoder_period_ms = (uint16_t)(dt_s * 1000.0f + 0.5f);
    if (encoder_period_ms < 1U) encoder_period_ms = 1U;
    if (encoder_period_ms > 100U) encoder_period_ms = 100U;

    actual_speed_counts = Encoder_GetSpeed(encoder_period_ms);
    actual_speed_rpm = actual_speed_counts * 60.0f /
                       ENCODER_COUNTS_PER_OUTPUT_REV;
    encoder_count = Encoder_GetCount();

    /* MPU6050 掉线时角度环失去反馈：实际角不再变化，
     * PID 会一直朝一个方向加码把电机跑飞，所以直接停机并复位积分；
     * 速度环不依赖 MPU，不受影响，等传感器恢复后会自动重新闭环。 */
    if (control_mode == CONTROL_MODE_ANGLE && ins_data()->sensor_ok == 0) {
        speed_target_rpm = 0.0f;
        pwm_output = 0.0f;
        PID_Reset(&pid_angle, actual_angle_deg);
        PID_Reset(&pid_speed, actual_speed_rpm);
        Motor_Stop();
        return;
    }

    if (control_mode == CONTROL_MODE_ANGLE) {
        float angle_error = Wrap180(target_angle_deg - actual_angle_deg);
        if (fabsf(angle_error) < CONTROL_ANGLE_DEADBAND_DEG &&
            fabsf(actual_speed_rpm) < CONTROL_SPEED_DEADBAND_RPM) {
            pid_angle.integral = 0.0f;
            pid_speed.integral = 0.0f;
            speed_target_rpm = 0.0f;
            pwm_output = 0.0f;
            Motor_Stop();
            return;
        } else {
            float effective_target = actual_angle_deg + angle_error;
            speed_target_rpm = PID_Calc(&pid_angle,
                                        effective_target,
                                        actual_angle_deg,
                                        dt_s);
        }
    } else {
        speed_target_rpm = target_speed_rpm;
    }

    pwm_output = PID_Calc(&pid_speed,
                          speed_target_rpm,
                          actual_speed_rpm,
                          dt_s);
    Motor_SetSpeed(pwm_output);
}

void Control_GetTelemetry(ControlTelemetry_t *telemetry)
{
    if (telemetry == NULL) {
        return;
    }

    ins_data_t *ins = ins_data();
    telemetry->mode = control_mode;
    telemetry->target_angle_deg = target_angle_deg;
    telemetry->actual_angle_deg = actual_angle_deg;
    telemetry->target_speed_rpm = speed_target_rpm;
    telemetry->actual_speed_rpm = actual_speed_rpm;
    telemetry->actual_speed_counts_per_second = actual_speed_counts;
    telemetry->encoder_count = encoder_count;
    telemetry->pwm_output = pwm_output;
    telemetry->roll_deg = ins->roll;
    telemetry->pitch_deg = ins->pitch;
    telemetry->yaw_deg = ins->yaw;
    memcpy(telemetry->gyro, ins->gyro, sizeof(telemetry->gyro));
    memcpy(telemetry->gyro_accel, ins->gyro_accel, sizeof(telemetry->gyro_accel));
    telemetry->imu_ok = ins->sensor_ok;
    telemetry->imu_read_errors = ins->read_errors;
}
