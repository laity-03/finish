#include "vofa.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "control.h"
#include "encoder.h"
#include "ins.h"
#include "usart.h"

static const uint8_t tail[4] = {0x00, 0x00, 0x80, 0x7f};

#define VOFA_CHANNEL_COUNT 14U

static uint8_t rx_byte = 0U;
static char rx_line[64];
static uint8_t rx_length = 0U;
static char ready_line[64];
static volatile uint8_t line_ready = 0U;

void Vofa_SendFloats(float *data, uint8_t count)
{
    if (data == NULL || count == 0U) {
        return;
    }

    HAL_UART_Transmit(&huart1, (uint8_t *)data,
                      (uint16_t)(count * sizeof(float)), 100U);
    HAL_UART_Transmit(&huart1, (uint8_t *)tail, sizeof(tail), 100U);
}

void Vofa_Init(void)
{
    rx_length = 0U;
    line_ready = 0U;
    memset(rx_line, 0, sizeof(rx_line));
    memset(ready_line, 0, sizeof(ready_line));
    (void)HAL_UART_Receive_IT(&huart1, &rx_byte, 1U);
}

static void Vofa_HandleCommand(char *line)
{
    while (*line != '\0' && isspace((unsigned char)*line)) {
        line++;
    }
    if (*line == '\0') {
        return;
    }

    char command = (char)toupper((unsigned char)line[0]);
    char *value_string = line + 1;
    while (*value_string != '\0' &&
           (isspace((unsigned char)*value_string) || *value_string == '=' ||
            *value_string == ':')) {
        value_string++;
    }

    if (command == 'A') {
        char *end = NULL;
        float value = strtof(value_string, &end);
        if (end != value_string) {
            Control_SetTargetAngle(value);
        }
    } else if (command == 'R' || command == 'S') {
        char *end = NULL;
        float value = strtof(value_string, &end);
        if (end != value_string) {
            Control_SetTargetSpeedRpm(value);
        }
    } else if (command == 'M') {
        char mode = (char)toupper((unsigned char)*value_string);
        if (mode == 'A') {
            Control_SetMode(CONTROL_MODE_ANGLE);
        } else if (mode == 'R' || mode == 'S') {
            Control_SetMode(CONTROL_MODE_SPEED);
        }
    } else if (command == 'Z') {
        Control_CaptureAngleZero();
    }
}

void Vofa_PollCommand(void)
{
    if (!line_ready) {
        return;
    }

    char command[64];
    __disable_irq();
    memcpy(command, ready_line, sizeof(command));
    command[sizeof(command) - 1U] = '\0';
    line_ready = 0U;
    __enable_irq();

    Vofa_HandleCommand(command);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        (void)HAL_UART_Receive_IT(&huart1, &rx_byte, 1U);
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        char received = (char)rx_byte;

        if (received == '\n' || received == '\r') {
            if (rx_length > 0U && !line_ready) {
                memcpy(ready_line, rx_line, rx_length);
                ready_line[rx_length] = '\0';
                line_ready = 1U;
            }
            rx_length = 0U;
        } else if (rx_length < (sizeof(rx_line) - 1U)) {
            rx_line[rx_length++] = received;
        } else {
            rx_length = 0U;
        }

        (void)HAL_UART_Receive_IT(&huart1, &rx_byte, 1U);
    }
}

void Vofa_SendTelemetry(void)
{
    ControlTelemetry_t telemetry;
    Control_GetTelemetry(&telemetry);


    float data[VOFA_CHANNEL_COUNT];
    data[0]  = (float)telemetry.mode;              //mode
    data[1]  = telemetry.target_angle_deg;         //目标
    data[2]  = telemetry.actual_angle_deg;         //实际
    data[3]  = telemetry.target_speed_rpm;         //目标
    data[4]  = telemetry.actual_speed_rpm;         //实际
    data[5]  = telemetry.pwm_output;               //PWM
    data[6]  = telemetry.roll_deg;                 //oll
    data[7]  = telemetry.pitch_deg;                //pitch
    data[8]  = telemetry.yaw_deg;                  //yaw
    data[9]  = telemetry.gyro[0];                  //角速度 X
    data[10] = telemetry.gyro[1];                  //角速度 Y
    data[11] = telemetry.gyro[2];                  //角速度 Z
    data[12] = (float)telemetry.encoder_count /
               ENCODER_COUNTS_PER_OUTPUT_REV;      //输出轴圈数
    data[13] = (float)telemetry.imu_ok;            /* 14 MPU6050 在线 1/0 */

    Vofa_SendFloats(data, (uint8_t)VOFA_CHANNEL_COUNT);
}

