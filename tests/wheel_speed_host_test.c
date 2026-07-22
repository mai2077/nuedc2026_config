#ifdef WHEEL_SPEED_HOST_TEST

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "wheel_speed.h"

static void testUnitConversions(void)
{
    assert(WHEEL_SPEED_counts10msToMmps(0) == 0);
    assert(WHEEL_SPEED_counts10msToMmps(1) == 57);
    assert(WHEEL_SPEED_counts10msToMmps(-1) == -57);
    assert(WHEEL_SPEED_mmpsToCountsQ8(400) == 1800);
    assert(WHEEL_SPEED_mmpsToCountsQ8(-400) == -1800);
}

static void testIndependentSignedControllers(void)
{
    WHEEL_SPEED_init();
    WHEEL_SPEED_setTargetsMmps(400, 0);
    WHEEL_SPEED_update10ms(0, 0);

    assert(WHEEL_SPEED_getRightTargetMmps() == 400);
    assert(WHEEL_SPEED_getLeftTargetMmps() == 0);
    assert(WHEEL_SPEED_getRightPwm() > 0);
    assert(WHEEL_SPEED_getLeftPwm() == 0);

    WHEEL_SPEED_setTargetsMmps(400, -400);
    WHEEL_SPEED_update10ms(0, 0);
    assert(WHEEL_SPEED_getRightPwm() > 0);
    assert(WHEEL_SPEED_getLeftPwm() < 0);
}

static void testZeroAndDirectionChangeResetIntegral(void)
{
    int16_t firstPositive;
    int16_t restartedPositive;
    int i;

    WHEEL_SPEED_init();
    assert(WHEEL_SPEED_setGainsX100(
        WHEEL_SPEED_WHEEL_RIGHT, 2000U, 200U) != 0U);
    WHEEL_SPEED_setTargetsMmps(400, 0);
    WHEEL_SPEED_update10ms(0, 0);
    firstPositive = WHEEL_SPEED_getRightPwm();

    for (i = 0; i < 100; ++i) {
        WHEEL_SPEED_update10ms(0, 0);
    }
    assert(WHEEL_SPEED_getRightPwm() == WHEEL_SPEED_PWM_LIMIT);

    WHEEL_SPEED_setTargetsMmps(0, 0);
    assert(WHEEL_SPEED_getRightPwm() == 0);
    WHEEL_SPEED_setTargetsMmps(400, 0);
    WHEEL_SPEED_update10ms(0, 0);
    restartedPositive = WHEEL_SPEED_getRightPwm();
    assert(restartedPositive == firstPositive);

    WHEEL_SPEED_setTargetsMmps(-400, 0);
    WHEEL_SPEED_update10ms(0, 0);
    assert(WHEEL_SPEED_getRightPwm() == -firstPositive);
}

static void testSaturationCanRecover(void)
{
    int i;

    WHEEL_SPEED_init();
    assert(WHEEL_SPEED_setGainsX100(
        WHEEL_SPEED_WHEEL_RIGHT, 2000U, 200U) != 0U);
    WHEEL_SPEED_setTargetsMmps(1200, 0);
    for (i = 0; i < 100; ++i) {
        WHEEL_SPEED_update10ms(0, 0);
    }
    assert(WHEEL_SPEED_getRightPwm() == WHEEL_SPEED_PWM_LIMIT);

    WHEEL_SPEED_update10ms(30, 0);
    assert(WHEEL_SPEED_getRightPwm() < WHEEL_SPEED_PWM_LIMIT);
}

static void testRuntimeGainsAndDiagnostics(void)
{
    WHEEL_SPEED_Diagnostics right;
    WHEEL_SPEED_Diagnostics left;

    WHEEL_SPEED_init();
    assert(WHEEL_SPEED_setGainsX100(
        WHEEL_SPEED_WHEEL_RIGHT, 1000U, 100U) != 0U);
    assert(WHEEL_SPEED_setGainsX100(
        WHEEL_SPEED_WHEEL_LEFT, 3000U, 300U) != 0U);
    assert(WHEEL_SPEED_setGainsX100(
        WHEEL_SPEED_WHEEL_RIGHT, WHEEL_SPEED_KP_X100_MAX + 1U, 0U) == 0U);
    assert(WHEEL_SPEED_setGainsX100(
        WHEEL_SPEED_WHEEL_RIGHT, 0U, WHEEL_SPEED_KI_X100_MAX + 1U) == 0U);

    WHEEL_SPEED_setTargetsMmps(400, 400);
    WHEEL_SPEED_update10ms(0, 0);
    WHEEL_SPEED_getDiagnostics(WHEEL_SPEED_WHEEL_RIGHT, &right);
    WHEEL_SPEED_getDiagnostics(WHEEL_SPEED_WHEEL_LEFT, &left);

    assert(right.kpX100 == 1000U);
    assert(right.kiX100 == 100U);
    assert(right.setpointMmps == 400);
    assert(right.inputMmps == 0);
    assert(right.errorMmps == 400);
    assert(right.proportionalPwm > 0);
    assert(right.integralPwm > 0);
    assert(right.derivativePwm == 0);
    assert(right.outputPwm == right.proportionalPwm + right.integralPwm);
    assert(left.outputPwm > right.outputPwm);

    assert(WHEEL_SPEED_setGainsX100(
        WHEEL_SPEED_WHEEL_RIGHT, 1000U, 100U) != 0U);
    WHEEL_SPEED_getDiagnostics(WHEEL_SPEED_WHEEL_RIGHT, &right);
    assert(right.outputPwm == 0);
    assert(right.integralPwm == 0);
}

int main(void)
{
    testUnitConversions();
    testIndependentSignedControllers();
    testZeroAndDirectionChangeResetIntegral();
    testSaturationCanRecover();
    testRuntimeGainsAndDiagnostics();
    puts("wheel speed tests passed");
    return 0;
}

#endif /* WHEEL_SPEED_HOST_TEST */
