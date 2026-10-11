#ifndef FINISH_CONTROL_H
#define FINISH_CONTROL_H

#include <stdint.h>


#define CONTROL_PERIOD_MS 10U
#define CONTROL_PERIOD_S  0.01f


#define CONTROL_ANGLE_USE_ROLL 1
#define CONTROL_ANGLE_SIGN     1.0f

#define CONTROL_TARGET_ANGLE_MAX_DEG   75.0f
#define CONTROL_TARGET_SPEED_MAX_RPM   120.0f
#define CONTROL_ANGLE_DEADBAND_DEG     1.0f
#define CONTROL_SPEED_DEADBAND_RPM     2.0f
#define CONTROL_KNOB_DEADBAND_DEG      0.2f
#define CONTROL_KNOB_SLEW_DEG_PER_S    90.0f
#define CONTROL_ZERO_CALIBRATION_SAMPLES 100U

typedef enum {
    CONTROL_MODE_ANGLE = 0,
    CONTROL_MODE_SPEED = 1
} ControlMode_t;

typedef struct {
    ControlMode_t mode;
    float target_angle_deg;
    float actual_angle_deg;
    float target_speed_rpm;
    float actual_speed_rpm;
    float actual_speed_counts_per_second;
    int16_t encoder_count;
    float pwm_output;
    float roll_deg;
    float pitch_deg;
    float yaw_deg;
    float gyro[3];
    float gyro_accel[3];
    uint8_t imu_ok;
    uint32_t imu_read_errors;
} ControlTelemetry_t;

void Control_Init(void);
void Control_Update(float dt_s);

void Control_SetMode(ControlMode_t mode);
ControlMode_t Control_GetMode(void);

void Control_SetTargetAngle(float angle_deg);
void Control_SetTargetSpeedRpm(float speed_rpm);
float Control_GetTargetAngle(void);
float Control_GetTargetSpeedRpm(void);

/* 将当前雪糕棒姿态记录为目标角 0 度，通常在上电初始化时调用。 */
void Control_CaptureAngleZero(void);

void Control_GetTelemetry(ControlTelemetry_t *telemetry);

#endif // FINISH_CONTROL_H
