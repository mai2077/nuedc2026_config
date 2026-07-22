#include "motor_polarity_test.h"

#include "key.h"
#include "tb6612.h"

static int16_t MOTOR_POLARITY_TEST_pairCommand(
    uint8_t pressedMask, uint8_t positiveKey, uint8_t negativeKey)
{
    uint8_t pair = pressedMask & (positiveKey | negativeKey);

    if (pair == positiveKey) {
        return TB6612_TEST_DUTY;
    }
    if (pair == negativeKey) {
        return -TB6612_TEST_DUTY;
    }
    return 0;
}

void MOTOR_POLARITY_TEST_init(void)
{
    TB6612_stopAll();
}

void MOTOR_POLARITY_TEST_applyKeys(uint8_t pressedMask)
{
    /* Motor 1 is the physically reversed right wheel. */
    int16_t motor1 = -MOTOR_POLARITY_TEST_pairCommand(
        pressedMask, KEY_MASK_KEY1, KEY_MASK_KEY2);
    int16_t motor2 = MOTOR_POLARITY_TEST_pairCommand(
        pressedMask, KEY_MASK_KEY3, KEY_MASK_KEY4);

    TB6612_setMotor1(motor1);
    TB6612_setMotor2(motor2);
}
