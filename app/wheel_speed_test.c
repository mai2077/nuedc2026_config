#include "wheel_speed_test.h"

#include "bsp/encoder.h"
#include "bsp/key.h"
#include "bsp/tb6612.h"
#include "control/wheel_speed.h"

static int16_t wheelSpeedTestPairTarget(
    uint8_t pressedMask, uint8_t positiveKey, uint8_t negativeKey)
{
    uint8_t pair = pressedMask & (positiveKey | negativeKey);

    if (pair == positiveKey) {
        return WHEEL_SPEED_TEST_TARGET_MMPS;
    }
    if (pair == negativeKey) {
        return -WHEEL_SPEED_TEST_TARGET_MMPS;
    }
    return 0;
}

void WHEEL_SPEED_TEST_init(void)
{
    WHEEL_SPEED_init();
    TB6612_stopAll();
}

void WHEEL_SPEED_TEST_applyKeys(uint8_t pressedMask)
{
    int16_t rightTarget = wheelSpeedTestPairTarget(
        pressedMask, KEY_MASK_KEY1, KEY_MASK_KEY2);
    int16_t leftTarget = wheelSpeedTestPairTarget(
        pressedMask, KEY_MASK_KEY3, KEY_MASK_KEY4);

    WHEEL_SPEED_setTargetsMmps(rightTarget, leftTarget);
}

void WHEEL_SPEED_TEST_update10ms(void)
{
    WHEEL_SPEED_update10ms(
        ENCODER_getRightSpeed10ms(), ENCODER_getLeftSpeed10ms());

    /* Motor 1 is mounted opposite to the software-defined forward direction. */
    TB6612_setMotor1((int16_t)-WHEEL_SPEED_getRightPwm());
    TB6612_setMotor2(WHEEL_SPEED_getLeftPwm());
}
