#ifdef MOTOR_POLARITY_HOST_TEST

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "key.h"
#include "motor_polarity_test.h"
#include "tb6612.h"

static int16_t gMotor1;
static int16_t gMotor2;
static uint32_t gStopAllCalls;

void TB6612_setMotor1(int16_t signedPwm)
{
    gMotor1 = signedPwm;
}

void TB6612_setMotor2(int16_t signedPwm)
{
    gMotor2 = signedPwm;
}

void TB6612_stopAll(void)
{
    ++gStopAllCalls;
    gMotor1 = 0;
    gMotor2 = 0;
}

static void testInitStopsBothMotors(void)
{
    gMotor1 = 123;
    gMotor2 = -456;
    gStopAllCalls = 0U;

    MOTOR_POLARITY_TEST_init();
    assert(gStopAllCalls == 1U);
    assert(gMotor1 == 0);
    assert(gMotor2 == 0);
}

static void testIndependentDirectionMapping(void)
{
    MOTOR_POLARITY_TEST_applyKeys(KEY_MASK_KEY1);
    assert(gMotor1 == -TB6612_TEST_DUTY);
    assert(gMotor2 == 0);

    MOTOR_POLARITY_TEST_applyKeys(KEY_MASK_KEY2 | KEY_MASK_KEY3);
    assert(gMotor1 == TB6612_TEST_DUTY);
    assert(gMotor2 == TB6612_TEST_DUTY);

    MOTOR_POLARITY_TEST_applyKeys(KEY_MASK_KEY1 | KEY_MASK_KEY4);
    assert(gMotor1 == -TB6612_TEST_DUTY);
    assert(gMotor2 == -TB6612_TEST_DUTY);
}

static void testConflictingPairsStopAffectedMotor(void)
{
    MOTOR_POLARITY_TEST_applyKeys(KEY_MASK_KEY1 | KEY_MASK_KEY2 |
                                  KEY_MASK_KEY3);
    assert(gMotor1 == 0);
    assert(gMotor2 == TB6612_TEST_DUTY);

    MOTOR_POLARITY_TEST_applyKeys(KEY_MASK_KEY1 | KEY_MASK_KEY2 |
                                  KEY_MASK_KEY3 | KEY_MASK_KEY4);
    assert(gMotor1 == 0);
    assert(gMotor2 == 0);

    MOTOR_POLARITY_TEST_applyKeys(0U);
    assert(gMotor1 == 0);
    assert(gMotor2 == 0);
}

int main(void)
{
    testInitStopsBothMotors();
    testIndependentDirectionMapping();
    testConflictingPairsStopAffectedMotor();
    puts("motor polarity tests passed");
    return 0;
}

#endif /* MOTOR_POLARITY_HOST_TEST */
