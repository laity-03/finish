#ifndef FINISH_KNOB_H
#define FINISH_KNOB_H

#include <stdint.h>

#include "main.h"

/*
 * Analog potentiometer module:
 * VCC -> 3.3 V, GND -> GND, OUT -> PA4 (ADC1_IN4).
 */
uint8_t Knob_Init(void);
void Knob_Update(void);

float Knob_GetPosition(void);
uint16_t Knob_GetRaw(void);
uint8_t Knob_IsReady(void);
uint8_t Knob_IsValid(void);

#endif // FINISH_KNOB_H
