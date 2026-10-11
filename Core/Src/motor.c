//
// Created by Laity on 2026/10/3.
//
#include "motor.h"
#include "tim.h"



#define MOTOR_PWM_MAX  100

void Motor_Init(void)
{

    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin   = GPIO_PIN_12 | GPIO_PIN_13;
    GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull  = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1);
    __HAL_TIM_MOE_ENABLE(&htim1);   // TIM1 是高级定时器，必须开 MOE 才有输出
    Motor_Stop();
}

void Motor_SetSpeed(float speed)
{

    if (speed > 0) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET);   // AIN1=1
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET); // AIN2=0
    } else if (speed < 0) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); // AIN1=0
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_SET);   // AIN2=1
        speed = -speed;
    } else {
        Motor_Stop();
        return;
    }

    if (speed > MOTOR_PWM_MAX) speed = MOTOR_PWM_MAX;

    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, (uint16_t)speed);
}

void Motor_Stop(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); // AIN1=0
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_13, GPIO_PIN_RESET); // AIN2=0
    __HAL_TIM_SET_COMPARE(&htim1, TIM_CHANNEL_1, 0);
}
