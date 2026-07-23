#include "angle_control.h"

#define ANGLE_CONTROL_DT_S (0.01f)
#define ANGLE_CONTROL_INTEGRAL_LIMIT (200.0f)
#define ANGLE_CONTROL_TARGET_CHANGE_EPSILON_DEG (0.001f)

typedef struct {
    ANGLE_CONTROL_State state;
    int16_t baseSpeedMmps;
    float kp;
    float ki;
    float kd;
    float targetYawDeg;
    float integralDegS;
    float previousErrorDeg;
    uint8_t hasPreviousError;
    uint8_t turnHoldTicks;
    uint8_t forwardFixedTicks;
    int8_t turnDirection;
    float previousTurnErrorDeg;
    ANGLE_CONTROL_Output output;
} ANGLE_CONTROL_Context;

static ANGLE_CONTROL_Context gAngleControl;

static float angleControlAbs(float value)
{
    return (value < 0.0f) ? -value : value;
}

static int16_t angleControlClampS16(int32_t value, int32_t lower, int32_t upper)
{
    if (value > upper) {
        return (int16_t)upper;
    }
    if (value < lower) {
        return (int16_t)lower;
    }
    return (int16_t)value;
}

static float angleControlClampFloat(float value, float lower, float upper)
{
    if (value > upper) {
        return upper;
    }
    if (value < lower) {
        return lower;
    }
    return value;
}

static void angleControlResetPidMemory(void)
{
    gAngleControl.integralDegS = 0.0f;
    gAngleControl.previousErrorDeg = 0.0f;
    gAngleControl.hasPreviousError = 0U;
}

static void angleControlResetTurnMemory(void)
{
    gAngleControl.turnHoldTicks = 0U;
    gAngleControl.forwardFixedTicks = 0U;
    gAngleControl.turnDirection = 0;
    gAngleControl.previousTurnErrorDeg = 0.0f;
}

static int16_t angleControlRoundTargetWithLimit(float value, int16_t limit)
{
    int32_t rounded = (value >= 0.0f) ?
        (int32_t)(value + 0.5f) : (int32_t)(value - 0.5f);

    return angleControlClampS16(rounded, (int32_t)-limit, (int32_t)limit);
}

static int16_t angleControlRoundForwardCorrection(float value)
{
    return angleControlRoundTargetWithLimit(
        value, ANGLE_CONTROL_FORWARD_CORRECTION_LIMIT_MMPS);
}

static float angleControlNormalizeYawDeg(float yawDeg)
{
    while (yawDeg > 180.0f) {
        yawDeg -= 360.0f;
    }
    while (yawDeg < -180.0f) {
        yawDeg += 360.0f;
    }
    return yawDeg;
}

static uint8_t angleControlTurnReachedTarget(float error)
{
    uint8_t crossedTarget = 0U;

    if (gAngleControl.turnDirection != 0) {
        crossedTarget = (uint8_t)(
            (((gAngleControl.previousTurnErrorDeg > 0.0f) && (error < 0.0f)) ||
             ((gAngleControl.previousTurnErrorDeg < 0.0f) && (error > 0.0f))) &&
            (angleControlAbs(error - gAngleControl.previousTurnErrorDeg) < 180.0f));
    }

    return (uint8_t)(
        (angleControlAbs(error) <= ANGLE_CONTROL_TURN_TOLERANCE_DEG) ||
        (crossedTarget != 0U));
}

static float angleControlPidStep(float error)
{
    float derivative;
    float turn;

    gAngleControl.integralDegS = angleControlClampFloat(
        gAngleControl.integralDegS + (error * ANGLE_CONTROL_DT_S),
        -ANGLE_CONTROL_INTEGRAL_LIMIT, ANGLE_CONTROL_INTEGRAL_LIMIT);
    derivative = (gAngleControl.hasPreviousError != 0U) ?
        ((error - gAngleControl.previousErrorDeg) / ANGLE_CONTROL_DT_S) :
        0.0f;
    gAngleControl.previousErrorDeg = error;
    gAngleControl.hasPreviousError = 1U;

    turn = (gAngleControl.kp * error) +
           (gAngleControl.ki * gAngleControl.integralDegS) +
           (gAngleControl.kd * derivative);
    return turn;
}

static void angleControlWriteOutput(
    int16_t rightTargetMmps, int16_t leftTargetMmps, float turnMmps)
{
    gAngleControl.output.rightTargetMmps = rightTargetMmps;
    gAngleControl.output.leftTargetMmps = leftTargetMmps;
    gAngleControl.output.state = gAngleControl.state;
    gAngleControl.output.targetYawDeg = gAngleControl.targetYawDeg;
    gAngleControl.output.turnMmps = turnMmps;
}

void ANGLE_CONTROL_init(void)
{
    gAngleControl.state = ANGLE_CONTROL_STATE_IDLE;
    gAngleControl.baseSpeedMmps = 0;
    gAngleControl.kp = 0.0f;
    gAngleControl.ki = 0.0f;
    gAngleControl.kd = 0.0f;
    gAngleControl.targetYawDeg = 0.0f;
    angleControlResetPidMemory();
    angleControlResetTurnMemory();
    gAngleControl.output.currentYawDeg = 0.0f;
    gAngleControl.output.errorDeg = 0.0f;
    angleControlWriteOutput(0, 0, 0.0f);
}

void ANGLE_CONTROL_disable(void)
{
    gAngleControl.state = ANGLE_CONTROL_STATE_IDLE;
    angleControlResetPidMemory();
    angleControlResetTurnMemory();
    angleControlWriteOutput(0, 0, 0.0f);
}

uint8_t ANGLE_CONTROL_isActive(void)
{
    return (uint8_t)(gAngleControl.state != ANGLE_CONTROL_STATE_IDLE);
}

void ANGLE_CONTROL_setBaseSpeedMmps(int16_t baseSpeedMmps)
{
    gAngleControl.baseSpeedMmps = baseSpeedMmps;
}

void ANGLE_CONTROL_setPid(float kp, float ki, float kd)
{
    uint8_t gainsChanged = 0U;

    if ((kp == kp) && (kp >= 0.0f) && (gAngleControl.kp != kp)) {
        gAngleControl.kp = kp;
        gainsChanged = 1U;
    }
    if ((ki == ki) && (ki >= 0.0f) && (gAngleControl.ki != ki)) {
        gAngleControl.ki = ki;
        gainsChanged = 1U;
    }
    if ((kd == kd) && (kd >= 0.0f) && (gAngleControl.kd != kd)) {
        gAngleControl.kd = kd;
        gainsChanged = 1U;
    }
    if (gainsChanged != 0U) {
        angleControlResetPidMemory();
    }
}

void ANGLE_CONTROL_setTargetYawDeg(float targetYawDeg)
{
    if (targetYawDeg != targetYawDeg) {
        return;
    }

    targetYawDeg = angleControlNormalizeYawDeg(targetYawDeg);
    if ((gAngleControl.state != ANGLE_CONTROL_STATE_IDLE) &&
        (angleControlAbs(ANGLE_CONTROL_wrapErrorDeg(
            targetYawDeg, gAngleControl.targetYawDeg)) <=
            ANGLE_CONTROL_TARGET_CHANGE_EPSILON_DEG)) {
        return;
    }

    gAngleControl.targetYawDeg = targetYawDeg;
    gAngleControl.state = ANGLE_CONTROL_STATE_TURN_OPEN_LOOP;
    angleControlResetPidMemory();
    angleControlResetTurnMemory();
    angleControlWriteOutput(0, 0, 0.0f);
}

float ANGLE_CONTROL_wrapErrorDeg(float targetYawDeg, float currentYawDeg)
{
    float error = targetYawDeg - currentYawDeg;

    while (error > 180.0f) {
        error -= 360.0f;
    }
    while (error < -180.0f) {
        error += 360.0f;
    }
    return error;
}

void ANGLE_CONTROL_update10ms(
    float currentYawDeg, ANGLE_CONTROL_Output *output)
{
    float error;
    float turn;
    float correctionLimit;
    int16_t turnTarget;

    gAngleControl.output.currentYawDeg = currentYawDeg;
    error = ANGLE_CONTROL_wrapErrorDeg(
        gAngleControl.targetYawDeg, currentYawDeg);
    gAngleControl.output.errorDeg = error;

    if (gAngleControl.state == ANGLE_CONTROL_STATE_IDLE) {
        angleControlWriteOutput(0, 0, 0.0f);
        ANGLE_CONTROL_getOutput(output);
        return;
    }

    if (gAngleControl.state == ANGLE_CONTROL_STATE_TURN_OPEN_LOOP) {
        if (angleControlTurnReachedTarget(error) != 0U) {
            gAngleControl.state = ANGLE_CONTROL_STATE_TURN_HOLD;
            gAngleControl.turnHoldTicks = 1U;
            gAngleControl.turnDirection = 0;
            gAngleControl.previousTurnErrorDeg = 0.0f;
            angleControlWriteOutput(0, 0, 0.0f);
            ANGLE_CONTROL_getOutput(output);
            return;
        }

        if (gAngleControl.turnDirection == 0) {
            gAngleControl.turnDirection = (error > 0.0f) ? 1 : -1;
        }
        gAngleControl.previousTurnErrorDeg = error;
        turnTarget = (int16_t)(
            (int16_t)gAngleControl.turnDirection *
            ANGLE_CONTROL_TURN_SPEED_MMPS);
        angleControlWriteOutput(turnTarget, (int16_t)-turnTarget,
            (float)turnTarget);
        ANGLE_CONTROL_getOutput(output);
        return;
    }

    if (gAngleControl.state == ANGLE_CONTROL_STATE_TURN_HOLD) {
        if (gAngleControl.turnHoldTicks < ANGLE_CONTROL_TURN_HOLD_TICKS) {
            ++gAngleControl.turnHoldTicks;
            angleControlWriteOutput(0, 0, 0.0f);
            ANGLE_CONTROL_getOutput(output);
            return;
        }

        gAngleControl.state = ANGLE_CONTROL_STATE_FORWARD_FIXED;
        gAngleControl.forwardFixedTicks = 1U;
        gAngleControl.turnDirection = 0;
        gAngleControl.previousTurnErrorDeg = 0.0f;
        angleControlWriteOutput(
            gAngleControl.baseSpeedMmps,
            gAngleControl.baseSpeedMmps, 0.0f);
        ANGLE_CONTROL_getOutput(output);
        return;
    }

    if (gAngleControl.state == ANGLE_CONTROL_STATE_FORWARD_FIXED) {
        if (gAngleControl.forwardFixedTicks <
            ANGLE_CONTROL_FORWARD_FIXED_TICKS) {
            ++gAngleControl.forwardFixedTicks;
            angleControlWriteOutput(
                gAngleControl.baseSpeedMmps,
                gAngleControl.baseSpeedMmps, 0.0f);
            ANGLE_CONTROL_getOutput(output);
            return;
        }

        gAngleControl.state = ANGLE_CONTROL_STATE_DRIVE_PID;
        angleControlResetPidMemory();
        angleControlResetTurnMemory();
    }

    if (gAngleControl.state == ANGLE_CONTROL_STATE_DRIVE_PID) {
        correctionLimit = angleControlAbs((float)gAngleControl.baseSpeedMmps);
        if (correctionLimit >
            (float)ANGLE_CONTROL_FORWARD_CORRECTION_LIMIT_MMPS) {
            correctionLimit =
                (float)ANGLE_CONTROL_FORWARD_CORRECTION_LIMIT_MMPS;
        }
        turn = angleControlPidStep(error);
        turn = angleControlClampFloat(turn, -correctionLimit, correctionLimit);
        turnTarget = angleControlRoundForwardCorrection(turn);
        angleControlWriteOutput(
            (int16_t)(gAngleControl.baseSpeedMmps + turnTarget),
            (int16_t)(gAngleControl.baseSpeedMmps - turnTarget), turn);
        ANGLE_CONTROL_getOutput(output);
        return;
    }

    angleControlWriteOutput(0, 0, 0.0f);
    ANGLE_CONTROL_getOutput(output);
}

void ANGLE_CONTROL_getOutput(ANGLE_CONTROL_Output *output)
{
    if (output == 0) {
        return;
    }
    *output = gAngleControl.output;
}

const char *ANGLE_CONTROL_stateName(ANGLE_CONTROL_State state)
{
    switch (state) {
        case ANGLE_CONTROL_STATE_DRIVE_PID:
            return "DRIVE_PID";
        case ANGLE_CONTROL_STATE_TURN_OPEN_LOOP:
            return "TURN_OPEN";
        case ANGLE_CONTROL_STATE_TURN_HOLD:
            return "TURN_HOLD";
        case ANGLE_CONTROL_STATE_FORWARD_FIXED:
            return "FORWARD_FIXED";
        case ANGLE_CONTROL_STATE_IDLE:
        default:
            return "IDLE";
    }
}
