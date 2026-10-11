#include "encoder.h"
#include "tim.h"

static int16_t last_count = 0;
static float filtered_speed = 0.0f;

void Encoder_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin  = GPIO_PIN_0 | GPIO_PIN_1;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_TIM_Encoder_Start(&htim2, TIM_CHANNEL_ALL);
    Encoder_Reset();
}

int16_t Encoder_GetCount(void)
{
    return (int16_t)__HAL_TIM_GET_COUNTER(&htim2);
}

float Encoder_GetSpeed(uint16_t period_ms)
{
    if (period_ms == 0U) {
        return 0.0f;
    }

    int16_t current_count = (int16_t)__HAL_TIM_GET_COUNTER(&htim2);
    int16_t delta = current_count - last_count;
    last_count = current_count;

    float speed = (float)delta * 1000.0f / (float)period_ms;
    speed *= ENCODER_DIRECTION_SIGN;
    filtered_speed += ENCODER_SPEED_LPF_ALPHA * (speed - filtered_speed);

    return filtered_speed;
}

float Encoder_GetSpeedRpm(uint16_t period_ms)
{
    float counts_per_second = Encoder_GetSpeed(period_ms);
    return counts_per_second * 60.0f / ENCODER_COUNTS_PER_OUTPUT_REV;
}

void Encoder_Reset(void)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0);
    last_count = 0;
    filtered_speed = 0.0f;
}
