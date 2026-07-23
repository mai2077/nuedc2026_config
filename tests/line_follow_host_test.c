#ifdef LINE_FOLLOW_HOST_TEST

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "line_follow.h"
#include "wheel_speed.h"

static void assertNear(float actual, float expected, float tolerance)
{
    float difference = actual - expected;
    if (difference < 0.0f) {
        difference = -difference;
    }
    assert(difference <= tolerance);
}

static LINE_FOLLOW_Output update(uint8_t rawMask)
{
    LINE_FOLLOW_Output output;
    LINE_FOLLOW_update10ms(rawMask, &output);
    return output;
}

static void testActiveHighSingleSensorPositions(void)
{
    static const uint8_t rawMasks[] = {0x01U, 0x02U, 0x04U, 0x08U, 0x10U};
    static const float errors[] = {43.0f, 15.0f, 0.0f, -15.0f, -43.0f};
    uint8_t i;

    for (i = 0U; i < 5U; ++i) {
        LINE_FOLLOW_init();
        LINE_FOLLOW_setBaseSpeedMmps(500);
        assertNear(update(rawMasks[i]).errorMm, errors[i], 0.001f);
    }
}

static void testWeightedAndSteeringDirection(void)
{
    LINE_FOLLOW_Output output;

    LINE_FOLLOW_init();
    LINE_FOLLOW_setBaseSpeedMmps(500);
    assert(LINE_FOLLOW_setPid(3.0f, 0.0f, 0.0f) != 0U);
    output = update(0x03U); /* OUT1 + OUT2 black: (43 + 15) / 2 = 29 */
    assertNear(output.errorMm, 29.0f, 0.001f);
    assert(output.rightTargetMmps == 413);
    assert(output.leftTargetMmps == 587);

    output = update(0x18U); /* OUT4 + OUT5 black: -29 */
    assert(output.rightTargetMmps > output.leftTargetMmps);
}

static void testCorrectionAndForwardOnlyLimits(void)
{
    LINE_FOLLOW_Output output;
    int16_t correctionLimit =
        (int16_t)LINE_FOLLOW_CORRECTION_MAX_MMPS;

    LINE_FOLLOW_init();
    LINE_FOLLOW_setBaseSpeedMmps(500);
    assert(LINE_FOLLOW_setPid(50.0f, 0.0f, 0.0f) != 0U);
    output = update(0x01U);
    assertNear(output.correctionMmps, (float)correctionLimit, 0.001f);
    assert(output.rightTargetMmps == (int16_t)(500 - correctionLimit));
    assert(output.leftTargetMmps == (int16_t)(500 + correctionLimit));

    LINE_FOLLOW_setBaseSpeedMmps(100);
    output = update(0x01U);
    assert(output.rightTargetMmps == 0);
    assert(output.leftTargetMmps == 200);
}

static void testLostLineIntersectionAndRecovery(void)
{
    LINE_FOLLOW_Output output;
    uint8_t tick;

    LINE_FOLLOW_init();
    LINE_FOLLOW_setBaseSpeedMmps(500);
    output = update(0x01U);
    for (tick = 1U; tick < LINE_FOLLOW_LOST_STOP_TICKS; ++tick) {
        output = update(0x00U);
        assert(output.state == LINE_FOLLOW_STATE_HOLD_LAST);
    }
    output = update(0x00U);
    assert(output.state == LINE_FOLLOW_STATE_LOST);
    assert(output.rightTargetMmps == 0);
    assert(output.leftTargetMmps == 0);

    output = update(0x04U);
    assert(output.state == LINE_FOLLOW_STATE_TRACKING);
    assert(output.rightTargetMmps == 500);
    assert(output.leftTargetMmps == 500);

    output = update(0x1FU);
    assert(output.state == LINE_FOLLOW_STATE_INTERSECTION);
    assertNear(output.errorMm, 0.0f, 0.001f);
}

static void testLiveGainValidationAndReset(void)
{
    LINE_FOLLOW_Output output;

    LINE_FOLLOW_init();
    LINE_FOLLOW_setBaseSpeedMmps(500);
    assert(LINE_FOLLOW_setPid(3.0f, 1.0f, 0.05f) != 0U);
    (void)update(0x01U);
    assert(LINE_FOLLOW_setPid(2.0f, 0.0f, 0.0f) != 0U);
    output = update(0x01U);
    assertNear(output.derivativeMmPerS, 0.0f, 0.001f);
    assertNear(output.integralMmS, 0.43f, 0.001f);

    assert(LINE_FOLLOW_setPid(-1.0f, 0.0f, 0.0f) == 0U);
    assert(LINE_FOLLOW_setPid(50.01f, 0.0f, 0.0f) == 0U);
    assert(LINE_FOLLOW_setPid(0.0f, 20.01f, 0.0f) == 0U);
    assert(LINE_FOLLOW_setPid(0.0f, 0.0f, 5.01f) == 0U);
    assert(LINE_FOLLOW_setPid(NAN, 0.0f, 0.0f) == 0U);
    assert(LINE_FOLLOW_setPid(INFINITY, 0.0f, 0.0f) == 0U);
}

static void testDynamicsResetPreservesConfiguration(void)
{
    LINE_FOLLOW_Output output;

    LINE_FOLLOW_init();
    LINE_FOLLOW_setBaseSpeedMmps(500);
    assert(LINE_FOLLOW_setPid(2.0f, 1.0f, 0.5f) != 0U);
    (void)update(0x01U);
    (void)update(0x10U);

    LINE_FOLLOW_resetDynamics();
    output = update(0x01U);
    assertNear(output.kp, 2.0f, 0.001f);
    assertNear(output.ki, 1.0f, 0.001f);
    assertNear(output.kd, 0.5f, 0.001f);
    assertNear(output.derivativeMmPerS, 0.0f, 0.001f);
    assertNear(output.integralMmS, 0.43f, 0.001f);
}

static void testCurveBiasFeedForward(void)
{
    LINE_FOLLOW_Output output;

    LINE_FOLLOW_init();
    LINE_FOLLOW_setBaseSpeedMmps(500);
    assert(LINE_FOLLOW_setPid(0.0f, 0.0f, 0.0f) != 0U);

    LINE_FOLLOW_setCurveBiasMmps(80);
    output = update(0x04U);
    assert(output.curveBiasMmps == 80);
    assert(output.rightTargetMmps == 580);
    assert(output.leftTargetMmps == 420);

    LINE_FOLLOW_setCurveBiasMmps(-60);
    output = update(0x04U);
    assert(output.curveBiasMmps == -60);
    assert(output.rightTargetMmps == 440);
    assert(output.leftTargetMmps == 560);

    output = update(0x1FU);
    assert(output.rightTargetMmps == 440);
    assert(output.leftTargetMmps == 560);

    LINE_FOLLOW_setBaseSpeedMmps(1450);
    LINE_FOLLOW_setCurveBiasMmps(500);
    output = update(0x04U);
    assert(output.rightTargetMmps == WHEEL_SPEED_MAX_TARGET_MMPS);
    assert(output.leftTargetMmps == 950);
}

int main(void)
{
    testActiveHighSingleSensorPositions();
    testWeightedAndSteeringDirection();
    testCorrectionAndForwardOnlyLimits();
    testLostLineIntersectionAndRecovery();
    testLiveGainValidationAndReset();
    testDynamicsResetPreservesConfiguration();
    testCurveBiasFeedForward();
    puts("line follow tests passed");
    return 0;
}

#endif /* LINE_FOLLOW_HOST_TEST */
