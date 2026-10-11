#include "key.h"

#define KEY_DEBOUNCE_TICKS  3U
#define KEY_LONG_PRESS_MS   800U

static void Key_EnablePortClock(GPIO_TypeDef *port)
{
    if (port == GPIOA) {
        __HAL_RCC_GPIOA_CLK_ENABLE();
    } else if (port == GPIOB) {
        __HAL_RCC_GPIOB_CLK_ENABLE();
    } else if (port == GPIOC) {
        __HAL_RCC_GPIOC_CLK_ENABLE();
    } else if (port == GPIOD) {
        __HAL_RCC_GPIOD_CLK_ENABLE();
    } else if (port == GPIOE) {
        __HAL_RCC_GPIOE_CLK_ENABLE();
    }
}

void Key_Init(Key_t *key, GPIO_TypeDef *port, uint16_t pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (key == NULL) {
        return;
    }

    Key_EnablePortClock(port);

    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(port, &GPIO_InitStruct);

    key->port = port;
    key->pin = pin;
    key->raw_last = 0U;
    key->stable_pressed = 0U;
    key->debounce_count = 0U;
    key->long_press_reported = 0U;
    key->press_start_tick = 0U;
}

KeyEvent_t Key_Update(Key_t *key)
{
    if (key == NULL) {
        return KEY_EVENT_NONE;
    }

    uint8_t raw_pressed =
        (HAL_GPIO_ReadPin(key->port, key->pin) == GPIO_PIN_RESET) ? 1U : 0U;

    if (raw_pressed != key->raw_last) {
        key->raw_last = raw_pressed;
        key->debounce_count = 0U;
    } else if (key->debounce_count < KEY_DEBOUNCE_TICKS) {
        key->debounce_count++;
    }

    if (key->debounce_count >= KEY_DEBOUNCE_TICKS &&
        raw_pressed != key->stable_pressed) {
        key->stable_pressed = raw_pressed;

        if (key->stable_pressed) {
            key->press_start_tick = HAL_GetTick();
            key->long_press_reported = 0U;
        } else if (!key->long_press_reported) {
            uint32_t press_duration = HAL_GetTick() - key->press_start_tick;
            return (press_duration >= KEY_LONG_PRESS_MS) ?
                   KEY_EVENT_LONG_PRESS : KEY_EVENT_SHORT_PRESS;
        }
    }

    if (key->stable_pressed && !key->long_press_reported &&
        (HAL_GetTick() - key->press_start_tick >= KEY_LONG_PRESS_MS)) {
        key->long_press_reported = 1U;
        return KEY_EVENT_LONG_PRESS;
    }

    return KEY_EVENT_NONE;
}
