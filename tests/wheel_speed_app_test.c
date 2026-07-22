#ifdef WHEEL_SPEED_APP_HOST_TEST

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "key.h"
#include "tb6612.h"
#include "wheel_speed.h"
#include "wheel_speed_test.h"

static int32_t gRightCounts10ms;
static int32_t gLeftCounts10ms;
static int16_t gMotor1Command;
static int16_t gMotor2Command;
static uint32_t gStopCalls;

int32_t ENCODER_getRightSpeed10ms(void)
{
    return gRightCounts10ms;
}

int32_t ENCODER_getLeftSpeed10ms(void)
{
    return gLeftCounts10ms;
}

void TB6612_setMotor1(int16_t signedPwm)
{
    gMotor1Command = signedPwm;
}

void TB6612_setMotor2(int16_t signedPwm)
{
    gMotor2Command = signedPwm;
}

void TB6612_stopAll(void)
{
    ++gStopCalls;
    gMotor1Command = 0;
    gMotor2Command = 0;
}

static void resetModel(void)
{
    gRightCounts10ms = 0;
    gLeftCounts10ms = 0;
    gMotor1Command = 123;
    gMotor2Command = -456;
    gStopCalls = 0U;
    WHEEL_SPEED_TEST_init();
}

static void testInitAndIndependentPositiveTargets(void)
{
    resetModel();
    assert(gStopCalls == 1U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 0);
    assert(WHEEL_SPEED_getLeftTargetMmps() == 0);

    WHEEL_SPEED_TEST_applyKeys(KEY_MASK_KEY1 | KEY_MASK_KEY3);
    assert(WHEEL_SPEED_getRightTargetMmps() == WHEEL_SPEED_TEST_TARGET_MMPS);
    assert(WHEEL_SPEED_getLeftTargetMmps() == WHEEL_SPEED_TEST_TARGET_MMPS);

    WHEEL_SPEED_TEST_update10ms();
    assert(gMotor1Command < 0);
    assert(gMotor2Command > 0);
    assert(gMotor1Command == -WHEEL_SPEED_getRightPwm());
    assert(gMotor2Command == WHEEL_SPEED_getLeftPwm());
}

static void testReverseAndConflictMapping(void)
{
    resetModel();
    WHEEL_SPEED_TEST_applyKeys(KEY_MASK_KEY2 | KEY_MASK_KEY4);
    assert(WHEEL_SPEED_getRightTargetMmps() == -WHEEL_SPEED_TEST_TARGET_MMPS);
    assert(WHEEL_SPEED_getLeftTargetMmps() == -WHEEL_SPEED_TEST_TARGET_MMPS);
    WHEEL_SPEED_TEST_update10ms();
    assert(gMotor1Command > 0);
    assert(gMotor2Command < 0);

    WHEEL_SPEED_TEST_applyKeys(KEY_MASK_KEY1 | KEY_MASK_KEY2 |
                               KEY_MASK_KEY3);
    assert(WHEEL_SPEED_getRightTargetMmps() == 0);
    assert(WHEEL_SPEED_getLeftTargetMmps() == WHEEL_SPEED_TEST_TARGET_MMPS);

    WHEEL_SPEED_TEST_applyKeys(0U);
    WHEEL_SPEED_TEST_update10ms();
    assert(gMotor1Command == 0);
    assert(gMotor2Command == 0);
}

int main(void)
{
    testInitAndIndependentPositiveTargets();
    testReverseAndConflictMapping();
    puts("wheel speed app tests passed");
    return 0;
}

#endif /* WHEEL_SPEED_APP_HOST_TEST */
