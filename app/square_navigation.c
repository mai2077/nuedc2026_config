#include "square_navigation.h"

#include "bsp/turn_indicator.h"
#include "bsp/track.h"
#include "control/angle_control.h"
#include "control/line_follow.h"

typedef struct {
    SQUARE_NAV_State state;
    uint8_t cornerConfirmTicks;
    uint8_t stopTicks;
    uint8_t imuFailureTicks;
    uint16_t turnTicks;
    float targetYawDeg;
    SQUARE_NAV_Output output;
} SQUARE_NAV_Context;

static SQUARE_NAV_Context gSquareNav;

static float squareNavNormalizeYawDeg(float yawDeg)
{
    while (yawDeg > 180.0f) {
        yawDeg -= 360.0f;
    }
    while (yawDeg < -180.0f) {
        yawDeg += 360.0f;
    }
    return yawDeg;
}

static uint8_t squareNavCornerDetected(uint8_t rawMask)
{
    const uint8_t relevant =
        TRACK_MASK_OUT3 | TRACK_MASK_OUT4 | TRACK_MASK_OUT5;
    const uint8_t expected = relevant;

    return (uint8_t)((rawMask & relevant) == expected);
}

static void squareNavWriteTargets(
    uint8_t rawMask, float currentYawDeg,
    int16_t rightTargetMmps, int16_t leftTargetMmps)
{
    gSquareNav.output.rightTargetMmps = rightTargetMmps;
    gSquareNav.output.leftTargetMmps = leftTargetMmps;
    gSquareNav.output.state = gSquareNav.state;
    gSquareNav.output.rawMask = (uint8_t)(rawMask & TRACK_MASK_ALL);
    gSquareNav.output.cornerConfirmTicks = gSquareNav.cornerConfirmTicks;
    gSquareNav.output.stopTicks = gSquareNav.stopTicks;
    gSquareNav.output.imuFailureTicks = gSquareNav.imuFailureTicks;
    gSquareNav.output.turnTicks = gSquareNav.turnTicks;
    gSquareNav.output.currentYawDeg = currentYawDeg;
    gSquareNav.output.targetYawDeg = gSquareNav.targetYawDeg;
}

static void squareNavWriteStopped(uint8_t rawMask, float currentYawDeg)
{
    squareNavWriteTargets(rawMask, currentYawDeg, 0, 0);
}

static void squareNavRunLine(uint8_t rawMask, float currentYawDeg)
{
    LINE_FOLLOW_Output lineOutput;

    LINE_FOLLOW_update10ms(rawMask, &lineOutput);
    squareNavWriteTargets(rawMask, currentYawDeg,
        lineOutput.rightTargetMmps, lineOutput.leftTargetMmps);
}

static void squareNavEnterFault(uint8_t rawMask, float currentYawDeg)
{
    ANGLE_CONTROL_disable();
    TURN_INDICATOR_stop();
    gSquareNav.state = SQUARE_NAV_STATE_FAULT;
    squareNavWriteStopped(rawMask, currentYawDeg);
}

void SQUARE_NAV_init(void)
{
    ANGLE_CONTROL_disable();
    TURN_INDICATOR_stop();
    gSquareNav.state = SQUARE_NAV_STATE_FOLLOW_ARMED;
    gSquareNav.cornerConfirmTicks = 0U;
    gSquareNav.stopTicks = 0U;
    gSquareNav.imuFailureTicks = 0U;
    gSquareNav.turnTicks = 0U;
    gSquareNav.targetYawDeg = 0.0f;
    squareNavWriteStopped(0U, 0.0f);
}

void SQUARE_NAV_update10ms(uint8_t rawMask, uint8_t yawValid,
    float currentYawDeg, SQUARE_NAV_Output *output)
{
    uint8_t cornerDetected = squareNavCornerDetected(rawMask);
    ANGLE_CONTROL_Output angleOutput;

    TURN_INDICATOR_update10ms();
    rawMask = (uint8_t)(rawMask & TRACK_MASK_ALL);

    if (gSquareNav.state == SQUARE_NAV_STATE_FAULT) {
        squareNavWriteStopped(rawMask, currentYawDeg);
        SQUARE_NAV_getOutput(output);
        return;
    }

    if (gSquareNav.state == SQUARE_NAV_STATE_FOLLOW_ARMED) {
        squareNavRunLine(rawMask, currentYawDeg);
        if (cornerDetected != 0U) {
            if (gSquareNav.cornerConfirmTicks <
                SQUARE_NAV_CORNER_CONFIRM_TICKS) {
                ++gSquareNav.cornerConfirmTicks;
            }
            if (gSquareNav.cornerConfirmTicks >=
                SQUARE_NAV_CORNER_CONFIRM_TICKS) {
                gSquareNav.state = SQUARE_NAV_STATE_STOPPING;
                gSquareNav.stopTicks = 1U;
                gSquareNav.imuFailureTicks = 0U;
                TURN_INDICATOR_start();
                squareNavWriteStopped(rawMask, currentYawDeg);
            }
        } else {
            gSquareNav.cornerConfirmTicks = 0U;
            gSquareNav.output.cornerConfirmTicks = 0U;
        }
        SQUARE_NAV_getOutput(output);
        return;
    }

    if (gSquareNav.state == SQUARE_NAV_STATE_STOPPING) {
        squareNavWriteStopped(rawMask, currentYawDeg);
        if (gSquareNav.stopTicks < SQUARE_NAV_STOP_TICKS) {
            ++gSquareNav.stopTicks;
            gSquareNav.output.stopTicks = gSquareNav.stopTicks;
            SQUARE_NAV_getOutput(output);
            return;
        }

        if (yawValid == 0U) {
            if (gSquareNav.imuFailureTicks < 255U) {
                ++gSquareNav.imuFailureTicks;
            }
            if (gSquareNav.imuFailureTicks >=
                SQUARE_NAV_IMU_FAILURE_LIMIT) {
                squareNavEnterFault(rawMask, currentYawDeg);
            } else {
                squareNavWriteStopped(rawMask, currentYawDeg);
            }
            SQUARE_NAV_getOutput(output);
            return;
        }

        gSquareNav.imuFailureTicks = 0U;
        gSquareNav.targetYawDeg = squareNavNormalizeYawDeg(
            currentYawDeg + SQUARE_NAV_LEFT_TURN_DEG);
        ANGLE_CONTROL_setTargetYawDeg(gSquareNav.targetYawDeg);
        gSquareNav.state = SQUARE_NAV_STATE_TURNING;
        gSquareNav.turnTicks = 1U;
        ANGLE_CONTROL_update10ms(currentYawDeg, &angleOutput);
        squareNavWriteTargets(rawMask, currentYawDeg,
            angleOutput.rightTargetMmps, angleOutput.leftTargetMmps);
        SQUARE_NAV_getOutput(output);
        return;
    }

    if (gSquareNav.state == SQUARE_NAV_STATE_TURNING) {
        if (yawValid == 0U) {
            if (gSquareNav.imuFailureTicks < 255U) {
                ++gSquareNav.imuFailureTicks;
            }
            if (gSquareNav.imuFailureTicks >=
                SQUARE_NAV_IMU_FAILURE_LIMIT) {
                squareNavEnterFault(rawMask, currentYawDeg);
            } else {
                squareNavWriteStopped(rawMask, currentYawDeg);
            }
            SQUARE_NAV_getOutput(output);
            return;
        }

        gSquareNav.imuFailureTicks = 0U;
        if (gSquareNav.turnTicks >= SQUARE_NAV_TURN_TIMEOUT_TICKS) {
            squareNavEnterFault(rawMask, currentYawDeg);
            SQUARE_NAV_getOutput(output);
            return;
        }
        ++gSquareNav.turnTicks;
        ANGLE_CONTROL_update10ms(currentYawDeg, &angleOutput);
        if (angleOutput.state == ANGLE_CONTROL_STATE_TURN_HOLD) {
            ANGLE_CONTROL_disable();
            LINE_FOLLOW_resetDynamics();
            gSquareNav.state = SQUARE_NAV_STATE_FOLLOW_WAIT_CLEAR;
            gSquareNav.cornerConfirmTicks = 0U;
            squareNavWriteStopped(rawMask, currentYawDeg);
        } else {
            squareNavWriteTargets(rawMask, currentYawDeg,
                angleOutput.rightTargetMmps, angleOutput.leftTargetMmps);
        }
        SQUARE_NAV_getOutput(output);
        return;
    }

    squareNavRunLine(rawMask, currentYawDeg);
    if (cornerDetected == 0U) {
        gSquareNav.state = SQUARE_NAV_STATE_FOLLOW_ARMED;
        gSquareNav.cornerConfirmTicks = 0U;
        gSquareNav.output.state = gSquareNav.state;
        gSquareNav.output.cornerConfirmTicks = 0U;
    }
    SQUARE_NAV_getOutput(output);
}

void SQUARE_NAV_getOutput(SQUARE_NAV_Output *output)
{
    if (output != 0) {
        *output = gSquareNav.output;
    }
}
