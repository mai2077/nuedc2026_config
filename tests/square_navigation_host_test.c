#ifdef SQUARE_NAV_HOST_TEST

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "app/square_navigation.h"
#include "control/angle_control.h"
#include "control/line_follow.h"

static uint16_t gIndicatorStartCount;
static uint16_t gIndicatorUpdateCount;
static uint16_t gIndicatorStopCount;

void TURN_INDICATOR_init(void) {}
void TURN_INDICATOR_start(void) { ++gIndicatorStartCount; }
void TURN_INDICATOR_update10ms(void) { ++gIndicatorUpdateCount; }
void TURN_INDICATOR_stop(void) { ++gIndicatorStopCount; }

static SQUARE_NAV_Output update(
    uint8_t rawMask, uint8_t yawValid, float yawDeg)
{
    SQUARE_NAV_Output output;

    SQUARE_NAV_update10ms(rawMask, yawValid, yawDeg, &output);
    return output;
}

static void resetModel(void)
{
    ANGLE_CONTROL_init();
    LINE_FOLLOW_init();
    LINE_FOLLOW_setBaseSpeedMmps(400);
    assert(LINE_FOLLOW_setPid(3.0f, 0.0f, 0.0f) != 0U);
    SQUARE_NAV_init();
    gIndicatorStartCount = 0U;
    gIndicatorUpdateCount = 0U;
    gIndicatorStopCount = 0U;
}

static void enterTurn(float stoppedYawDeg)
{
    SQUARE_NAV_Output output;
    uint8_t tick;

    output = update(0x1CU, 1U, stoppedYawDeg);
    assert(output.state == SQUARE_NAV_STATE_FOLLOW_ARMED);
    output = update(0x1CU, 1U, stoppedYawDeg);
    assert(output.state == SQUARE_NAV_STATE_STOPPING);
    assert(gIndicatorStartCount == 1U);
    assert(output.rightTargetMmps == 0);
    assert(output.leftTargetMmps == 0);

    for (tick = 1U; tick < SQUARE_NAV_STOP_TICKS; ++tick) {
        output = update(0x1CU, 1U, stoppedYawDeg);
        assert(output.state == SQUARE_NAV_STATE_STOPPING);
        assert(output.rightTargetMmps == 0);
        assert(output.leftTargetMmps == 0);
    }

    output = update(0x1CU, 1U, stoppedYawDeg);
    assert(output.state == SQUARE_NAV_STATE_TURNING);
    assert(output.rightTargetMmps == ANGLE_CONTROL_TURN_SPEED_MMPS);
    assert(output.leftTargetMmps == -ANGLE_CONTROL_TURN_SPEED_MMPS);
}

static void testCornerStopsTurnsAndRearms(void)
{
    SQUARE_NAV_Output output;

    resetModel();
    enterTurn(10.0f);

    output = update(0x1CU, 1U, 55.0f);
    assert(output.state == SQUARE_NAV_STATE_TURNING);
    assert(output.rightTargetMmps == ANGLE_CONTROL_TURN_SPEED_MMPS);
    assert(output.leftTargetMmps == -ANGLE_CONTROL_TURN_SPEED_MMPS);

    output = update(0x1CU, 1U, 96.0f);
    assert(output.state == SQUARE_NAV_STATE_FOLLOW_WAIT_CLEAR);
    assert(output.rightTargetMmps == 0);
    assert(output.leftTargetMmps == 0);

    output = update(0x1CU, 1U, 96.0f);
    assert(output.state == SQUARE_NAV_STATE_FOLLOW_WAIT_CLEAR);
    assert(output.rightTargetMmps != 0);
    assert(output.leftTargetMmps != 0);

    output = update(0x04U, 1U, 96.0f);
    assert(output.state == SQUARE_NAV_STATE_FOLLOW_ARMED);
    assert(output.rightTargetMmps == 400);
    assert(output.leftTargetMmps == 400);

    output = update(0x1CU, 1U, 96.0f);
    assert(output.state == SQUARE_NAV_STATE_FOLLOW_ARMED);
    output = update(0x1CU, 1U, 96.0f);
    assert(output.state == SQUARE_NAV_STATE_STOPPING);
}

static void testCornerPatternIgnoresRightSensors(void)
{
    SQUARE_NAV_Output output;

    resetModel();
    output = update(0x1FU, 1U, 0.0f); /* Right sensors are ignored. */
    assert(output.state == SQUARE_NAV_STATE_FOLLOW_ARMED);
    output = update(0x1FU, 1U, 0.0f);
    assert(output.state == SQUARE_NAV_STATE_STOPPING);

    resetModel();
    output = update(0x18U, 1U, 0.0f); /* OUT3 is white: not a corner. */
    output = update(0x18U, 1U, 0.0f);
    assert(output.state == SQUARE_NAV_STATE_FOLLOW_ARMED);
}

static void testYawWraparound(void)
{
    SQUARE_NAV_Output output;

    resetModel();
    enterTurn(170.0f);
    SQUARE_NAV_getOutput(&output);
    assert(output.targetYawDeg == -100.0f);

    output = update(0x1CU, 1U, -104.0f);
    assert(output.state == SQUARE_NAV_STATE_FOLLOW_WAIT_CLEAR);
}

static void testImuFailureStopsAndLatchesFault(void)
{
    SQUARE_NAV_Output output;
    uint8_t failure;

    resetModel();
    enterTurn(0.0f);

    output = update(0x1CU, 0U, 0.0f);
    assert(output.state == SQUARE_NAV_STATE_TURNING);
    assert(output.rightTargetMmps == 0);
    assert(output.leftTargetMmps == 0);
    output = update(0x1CU, 1U, 20.0f);
    assert(output.state == SQUARE_NAV_STATE_TURNING);
    assert(output.imuFailureTicks == 0U);
    assert(output.rightTargetMmps == ANGLE_CONTROL_TURN_SPEED_MMPS);

    for (failure = 0U; failure < SQUARE_NAV_IMU_FAILURE_LIMIT; ++failure) {
        output = update(0x1CU, 0U, 20.0f);
    }
    assert(output.state == SQUARE_NAV_STATE_FAULT);
    assert(gIndicatorStopCount == 1U);
    assert(output.rightTargetMmps == 0);
    assert(output.leftTargetMmps == 0);
}

static void testTurnTimeoutStops(void)
{
    SQUARE_NAV_Output output;
    uint16_t tick;

    resetModel();
    enterTurn(0.0f);
    for (tick = 1U; tick < SQUARE_NAV_TURN_TIMEOUT_TICKS; ++tick) {
        output = update(0x1CU, 1U, 0.0f);
        assert(output.state == SQUARE_NAV_STATE_TURNING);
    }
    output = update(0x1CU, 1U, 0.0f);
    assert(output.state == SQUARE_NAV_STATE_FAULT);
    assert(output.rightTargetMmps == 0);
    assert(output.leftTargetMmps == 0);
}

int main(void)
{
    testCornerStopsTurnsAndRearms();
    testCornerPatternIgnoresRightSensors();
    testYawWraparound();
    testImuFailureStopsAndLatchesFault();
    testTurnTimeoutStops();
    puts("square navigation tests passed");
    return 0;
}

#endif /* SQUARE_NAV_HOST_TEST */
