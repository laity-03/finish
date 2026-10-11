#ifndef FINISH_KEY_H
#define FINISH_KEY_H

#include "main.h"

typedef enum {
    KEY_EVENT_NONE = 0,
    KEY_EVENT_SHORT_PRESS,
    KEY_EVENT_LONG_PRESS
} KeyEvent_t;

typedef struct {
    GPIO_TypeDef *port;
    uint16_t pin;
    uint8_t raw_last;
    uint8_t stable_pressed;
    uint8_t debounce_count;
    uint8_t long_press_reported;
    uint32_t press_start_tick;
} Key_t;

void Key_Init(Key_t *key, GPIO_TypeDef *port, uint16_t pin);
KeyEvent_t Key_Update(Key_t *key);

#endif // FINISH_KEY_H
