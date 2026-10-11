/*
 * Host-side behavior test for the two on-board keys.
 *
 * It drives the real control.c state machine at the 100 Hz control period and
 * checks:
 *   - angle key short press cycles the ANGLE target 0 -> 10 -> 30 -> 60 -> 0
 *   - angle key long press returns the target to 0
 *   - speed key short press still cycles the SPEED presets
 *   - speed key long press still returns to ANGLE mode
 */
#include <math.h>
#include <stdio.h>

#include "control.h"
#include "key.h"
#include "test_support.h"

static int failures = 0;

static void CheckFloat(const char *name, float actual, float expected)
{
    if (fabsf(actual - expected) > 0.001f) {
        printf("FAIL %-34s expected %8.2f, got %8.2f\n", name, expected, actual);
        failures++;
    } else {
        printf("ok   %-34s %8.2f\n", name, actual);
    }
}

static void CheckInt(const char *name, int actual, int expected)
{
    if (actual != expected) {
        printf("FAIL %-34s expected %8d, got %8d\n", name, expected, actual);
        failures++;
    } else {
        printf("ok   %-34s %8d\n", name, actual);
    }
}

/* One control tick = 10 ms, the same period the firmware uses. */
static void StepTicks(int count)
{
    for (int i = 0; i < count; i++) {
        Test_AdvanceMs(10U);
        Control_Update(0.01f);
    }
}

static void ShortPress(uint16_t pin)
{
    Test_SetPinPressed(pin, 1);
    StepTicks(8);                /* hold about 80 ms */
    Test_SetPinPressed(pin, 0);
    StepTicks(8);                /* debounce + release event */
}

static void LongPress(uint16_t pin)
{
    Test_SetPinPressed(pin, 1);
    StepTicks(90);               /* hold 900 ms, longer than 800 ms */
    Test_SetPinPressed(pin, 0);
    StepTicks(8);
}

int main(void)
{
    Test_SetSensorOk(1U);
    Test_SetRollDeg(0.0f);
    Control_Init();

    printf("== initial state ==\n");
    CheckFloat("target angle", Control_GetTargetAngle(), 0.0f);
    CheckInt("control mode (0=ANGLE)", (int)Control_GetMode(), CONTROL_MODE_ANGLE);

    printf("== angle key: short press cycle ==\n");
    ShortPress(TEST_KEY_ANGLE_PIN);
    CheckFloat("1st press target angle", Control_GetTargetAngle(), 10.0f);
    ShortPress(TEST_KEY_ANGLE_PIN);
    CheckFloat("2nd press target angle", Control_GetTargetAngle(), 30.0f);
    ShortPress(TEST_KEY_ANGLE_PIN);
    CheckFloat("3rd press target angle", Control_GetTargetAngle(), 60.0f);
    ShortPress(TEST_KEY_ANGLE_PIN);
    CheckFloat("4th press target angle", Control_GetTargetAngle(), 0.0f);
    ShortPress(TEST_KEY_ANGLE_PIN);
    CheckFloat("5th press target angle", Control_GetTargetAngle(), 10.0f);

    printf("== angle key: long press ==\n");
    LongPress(TEST_KEY_ANGLE_PIN);
    CheckFloat("long press target angle", Control_GetTargetAngle(), 0.0f);
    CheckInt("mode after long press", (int)Control_GetMode(), CONTROL_MODE_ANGLE);

    printf("== speed key still works ==\n");
    ShortPress(TEST_KEY_SPEED_PIN);
    CheckInt("mode after speed short press", (int)Control_GetMode(), CONTROL_MODE_SPEED);
    CheckFloat("speed preset 1", Control_GetTargetSpeedRpm(), 30.0f);
    ShortPress(TEST_KEY_SPEED_PIN);
    CheckFloat("speed preset 2", Control_GetTargetSpeedRpm(), 60.0f);
    LongPress(TEST_KEY_SPEED_PIN);
    CheckInt("mode after speed long press", (int)Control_GetMode(), CONTROL_MODE_ANGLE);

    printf("== angle key also works from SPEED mode ==\n");
    ShortPress(TEST_KEY_SPEED_PIN);
    CheckInt("back in speed mode", (int)Control_GetMode(), CONTROL_MODE_SPEED);
    ShortPress(TEST_KEY_ANGLE_PIN);
    CheckInt("angle key switches to ANGLE", (int)Control_GetMode(), CONTROL_MODE_ANGLE);
    CheckFloat("next angle preset", Control_GetTargetAngle(), 10.0f);

    printf("== Z command resets the preset index ==\n");
    Control_SetTargetAngle(60.0f);
    Control_CaptureAngleZero();
    ShortPress(TEST_KEY_ANGLE_PIN);
    CheckFloat("first press after zero", Control_GetTargetAngle(), 10.0f);

    if (failures == 0) {
        printf("\nALL CHECKS PASSED\n");
        return 0;
    }

    printf("\n%d CHECK(S) FAILED\n", failures);
    return 1;
}
