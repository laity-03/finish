/*
 * Host-side stand-ins for the STM32 HAL and for the encoder / motor / INS
 * modules that control.c depends on. The real control.c and key.c are used
 * unchanged.
 */
#include "stm32f1xx_hal.h"

#include "test_support.h"

#include "encoder.h"
#include "ins.h"
#include "motor.h"

GPIO_TypeDef test_gpio_a;
GPIO_TypeDef test_gpio_b;
GPIO_TypeDef test_gpio_c;
GPIO_TypeDef test_gpio_d;
GPIO_TypeDef test_gpio_e;

static uint32_t tick_ms = 0U;
static uint16_t pressed_pins = 0U;
static ins_data_t ins_state;

/* ---------------------------- HAL subset ---------------------------- */

void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init)
{
    (void)GPIOx;
    (void)GPIO_Init;
}

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    (void)GPIOx;
    return (pressed_pins & GPIO_Pin) != 0U ? GPIO_PIN_RESET : GPIO_PIN_SET;
}

uint32_t HAL_GetTick(void)
{
    return tick_ms;
}

void HAL_Delay(uint32_t Delay)
{
    tick_ms += Delay;
}

/* --------------------------- test helpers --------------------------- */

void Test_SetPinPressed(uint16_t pin, int pressed)
{
    if (pressed) {
        pressed_pins |= pin;
    } else {
        pressed_pins &= (uint16_t)~pin;
    }
}

void Test_AdvanceMs(uint32_t ms)
{
    tick_ms += ms;
}

void Test_SetSensorOk(uint8_t ok)
{
    ins_state.sensor_ok = ok;
}

void Test_SetRollDeg(float deg)
{
    ins_state.roll = deg;
}

/* --------------------------- INS stubs ------------------------------ */

void ins_init(void)
{
}

void ins_update(void)
{
}

ins_data_t *ins_data(void)
{
    return &ins_state;
}

const float *ins_get_gyro(void)
{
    return ins_state.gyro;
}

const float *ins_get_gyro_accel(void)
{
    return ins_state.gyro_accel;
}

/* ------------------------- encoder stubs ---------------------------- */

void Encoder_Init(void)
{
}

void Encoder_Reset(void)
{
}

int16_t Encoder_GetCount(void)
{
    return 0;
}

float Encoder_GetSpeed(uint16_t period_ms)
{
    (void)period_ms;
    return 0.0f;
}

float Encoder_GetSpeedRpm(uint16_t period_ms)
{
    (void)period_ms;
    return 0.0f;
}

/* -------------------------- motor stubs ----------------------------- */

void Motor_Init(void)
{
}

void Motor_SetSpeed(float speed)
{
    (void)speed;
}

void Motor_Stop(void)
{
}
