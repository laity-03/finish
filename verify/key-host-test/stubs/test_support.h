#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include <stdint.h>

/* Pin masks must match Core/Inc/main.h (KEY_SPEED=PA2, KEY_ANGLE=PA3). */
#define TEST_KEY_SPEED_PIN 0x0004U
#define TEST_KEY_ANGLE_PIN 0x0008U

void Test_SetPinPressed(uint16_t pin, int pressed);
void Test_AdvanceMs(uint32_t ms);
void Test_SetSensorOk(uint8_t ok);
void Test_SetRollDeg(float deg);

#endif /* TEST_SUPPORT_H */
