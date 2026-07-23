#include "line_follow.h"

#include "wheel_speed.h"

#define LINE_FOLLOW_DT_S             (0.01f)
#define LINE_FOLLOW_INTEGRAL_LIMIT   (200.0f)
#define LINE_FOLLOW_SENSOR_COUNT     (5U)
#define LINE_FOLLOW_ACTIVE_MASK      (0x1FU)

typedef struct {
    int16_t baseSpeedMmps;
    int16_t curveBiasMmps;
    float kp;
    float ki;
    float kd;
    float integralMmS;
    float previousErrorMm;
    float lastValidErrorMm;
    uint8_t hasPreviousError;
    uint8_t hasLastValidError;
    uint8_t lostLineTicks;
    LINE_FOLLOW_Output output;
} LINE_FOLLOW_Context;

static const int8_t gSensorPositionMm[LINE_FOLLOW_SENSOR_COUNT] = {
    43, 15, 0, -15, -43
};

static LINE_FOLLOW_Context gLineFollow;

static float lineFollowClampFloat(float value, float lower, float upper)
{
    if (value > upper) {
        return upper;
    }
    if (value < lower) {
        return lower;
    }
    return value;
}

static int16_t lineFollowClampTarget(int32_t value)
{
    if (value > WHEEL_SPEED_MAX_TARGET_MMPS) {
        return WHEEL_SPEED_MAX_TARGET_MMPS;
    }
    if (value < 0) {
        return 0;
    }
    return (int16_t)value;
}

static int16_t lineFollowRound(float value)
{
    return (int16_t)((value >= 0.0f) ?
        (int32_t)(value + 0.5f) : (int32_t)(value - 0.5f));
}

static void lineFollowResetPidMemory(void)
{
    gLineFollow.integralMmS = 0.0f;
    gLineFollow.previousErrorMm = 0.0f;
    gLineFollow.hasPreviousError = 0U;
}

static void lineFollowWriteCommonOutput(
    uint8_t rawMask, uint8_t activeMask, LINE_FOLLOW_State state)
{
    gLineFollow.output.rawMask = rawMask;
    gLineFollow.output.activeMask = activeMask;
    gLineFollow.output.lostLineTicks = gLineFollow.lostLineTicks;
    gLineFollow.output.state = state;
    gLineFollow.output.kp = gLineFollow.kp;
    gLineFollow.output.ki = gLineFollow.ki;
    gLineFollow.output.kd = gLineFollow.kd;
    gLineFollow.output.curveBiasMmps = gLineFollow.curveBiasMmps;
}

static void lineFollowWriteStoppedOutput(
    uint8_t rawMask, uint8_t activeMask, LINE_FOLLOW_State state)
{
    lineFollowWriteCommonOutput(rawMask, activeMask, state);
    gLineFollow.output.integralMmS = gLineFollow.integralMmS;
    gLineFollow.output.derivativeMmPerS = 0.0f;
    gLineFollow.output.proportionalMmps = 0.0f;
    gLineFollow.output.integralMmps = 0.0f;
    gLineFollow.output.derivativeMmps = 0.0f;
    gLineFollow.output.correctionMmps = 0.0f;
    gLineFollow.output.rightTargetMmps = 0;
    gLineFollow.output.leftTargetMmps = 0;
}

static void lineFollowWriteStraightOutput(
    uint8_t rawMask, uint8_t activeMask, LINE_FOLLOW_State state)
{
    lineFollowWriteCommonOutput(rawMask, activeMask, state);
    gLineFollow.output.errorMm = 0.0f;
    gLineFollow.output.integralMmS = 0.0f;
    gLineFollow.output.derivativeMmPerS = 0.0f;
    gLineFollow.output.proportionalMmps = 0.0f;
    gLineFollow.output.integralMmps = 0.0f;
    gLineFollow.output.derivativeMmps = 0.0f;
    gLineFollow.output.correctionMmps = 0.0f;
    gLineFollow.output.rightTargetMmps = lineFollowClampTarget(
        (int32_t)gLineFollow.baseSpeedMmps + gLineFollow.curveBiasMmps);
    gLineFollow.output.leftTargetMmps = lineFollowClampTarget(
        (int32_t)gLineFollow.baseSpeedMmps - gLineFollow.curveBiasMmps);
}

static float lineFollowPositionError(uint8_t activeMask)
{
    int16_t positionSum = 0;
    uint8_t activeCount = 0U;
    uint8_t sensor;

    for (sensor = 0U; sensor < LINE_FOLLOW_SENSOR_COUNT; ++sensor) {
        if ((activeMask & (uint8_t)(1U << sensor)) != 0U) {
            positionSum = (int16_t)(positionSum + gSensorPositionMm[sensor]);
            ++activeCount;
        }
    }

    return (activeCount == 0U) ?
        0.0f : ((float)positionSum / (float)activeCount);
}

static void lineFollowRunPid(uint8_t rawMask, uint8_t activeMask,
    float errorMm, LINE_FOLLOW_State state)
{
    float derivativeMmPerS;
    float proportionalMmps;
    float integralMmps;
    float derivativeMmps;
    float correctionMmps;
    float correctionLimit;
    int16_t roundedCorrection;

    gLineFollow.integralMmS = lineFollowClampFloat(
        gLineFollow.integralMmS + (errorMm * LINE_FOLLOW_DT_S),
        -LINE_FOLLOW_INTEGRAL_LIMIT, LINE_FOLLOW_INTEGRAL_LIMIT);
    derivativeMmPerS = (gLineFollow.hasPreviousError != 0U) ?
        ((errorMm - gLineFollow.previousErrorMm) / LINE_FOLLOW_DT_S) : 0.0f;
    gLineFollow.previousErrorMm = errorMm;
    gLineFollow.hasPreviousError = 1U;

    proportionalMmps = gLineFollow.kp * errorMm;
    integralMmps = gLineFollow.ki * gLineFollow.integralMmS;
    derivativeMmps = gLineFollow.kd * derivativeMmPerS;
    correctionMmps = proportionalMmps + integralMmps + derivativeMmps;
    correctionLimit = (float)gLineFollow.baseSpeedMmps;
    if (correctionLimit > (float)LINE_FOLLOW_CORRECTION_MAX_MMPS) {
        correctionLimit = (float)LINE_FOLLOW_CORRECTION_MAX_MMPS;
    }
    correctionMmps = lineFollowClampFloat(
        correctionMmps, -correctionLimit, correctionLimit);
    roundedCorrection = lineFollowRound(correctionMmps);

    lineFollowWriteCommonOutput(rawMask, activeMask, state);
    gLineFollow.output.errorMm = errorMm;
    gLineFollow.output.integralMmS = gLineFollow.integralMmS;
    gLineFollow.output.derivativeMmPerS = derivativeMmPerS;
    gLineFollow.output.proportionalMmps = proportionalMmps;
    gLineFollow.output.integralMmps = integralMmps;
    gLineFollow.output.derivativeMmps = derivativeMmps;
    gLineFollow.output.correctionMmps = correctionMmps;
    gLineFollow.output.rightTargetMmps = lineFollowClampTarget(
        (int32_t)gLineFollow.baseSpeedMmps + gLineFollow.curveBiasMmps -
            roundedCorrection);
    gLineFollow.output.leftTargetMmps = lineFollowClampTarget(
        (int32_t)gLineFollow.baseSpeedMmps - gLineFollow.curveBiasMmps +
            roundedCorrection);
}

void LINE_FOLLOW_init(void)
{
    gLineFollow.baseSpeedMmps = 0;
    gLineFollow.curveBiasMmps = 0;
    gLineFollow.kp = LINE_FOLLOW_DEFAULT_KP;
    gLineFollow.ki = LINE_FOLLOW_DEFAULT_KI;
    gLineFollow.kd = LINE_FOLLOW_DEFAULT_KD;
    gLineFollow.lastValidErrorMm = 0.0f;
    gLineFollow.hasLastValidError = 0U;
    gLineFollow.lostLineTicks = 0U;
    lineFollowResetPidMemory();
    gLineFollow.output.errorMm = 0.0f;
    lineFollowWriteStoppedOutput(
        0U, 0U, LINE_FOLLOW_STATE_LOST);
}

void LINE_FOLLOW_setCurveBiasMmps(int16_t curveBiasMmps)
{
    if (gLineFollow.curveBiasMmps != curveBiasMmps) {
        gLineFollow.curveBiasMmps = curveBiasMmps;
        LINE_FOLLOW_resetDynamics();
    }
}

void LINE_FOLLOW_setBaseSpeedMmps(int16_t baseSpeedMmps)
{
    if (baseSpeedMmps < 0) {
        baseSpeedMmps = 0;
    } else if (baseSpeedMmps > WHEEL_SPEED_MAX_TARGET_MMPS) {
        baseSpeedMmps = WHEEL_SPEED_MAX_TARGET_MMPS;
    }
    gLineFollow.baseSpeedMmps = baseSpeedMmps;
}

uint8_t LINE_FOLLOW_pidIsValid(float kp, float ki, float kd)
{
    return (uint8_t)(
        (kp == kp) && (ki == ki) && (kd == kd) &&
        (kp >= 0.0f) && (kp <= LINE_FOLLOW_KP_MAX) &&
        (ki >= 0.0f) && (ki <= LINE_FOLLOW_KI_MAX) &&
        (kd >= 0.0f) && (kd <= LINE_FOLLOW_KD_MAX));
}

uint8_t LINE_FOLLOW_setPid(float kp, float ki, float kd)
{
    if (LINE_FOLLOW_pidIsValid(kp, ki, kd) == 0U) {
        return 0U;
    }

    if ((gLineFollow.kp != kp) ||
        (gLineFollow.ki != ki) ||
        (gLineFollow.kd != kd)) {
        gLineFollow.kp = kp;
        gLineFollow.ki = ki;
        gLineFollow.kd = kd;
        lineFollowResetPidMemory();
    }
    return 1U;
}

void LINE_FOLLOW_resetDynamics(void)
{
    gLineFollow.lastValidErrorMm = 0.0f;
    gLineFollow.hasLastValidError = 0U;
    gLineFollow.lostLineTicks = 0U;
    lineFollowResetPidMemory();
    gLineFollow.output.errorMm = 0.0f;
    lineFollowWriteStoppedOutput(
        0U, 0U, LINE_FOLLOW_STATE_HOLD_LAST);
}

void LINE_FOLLOW_update10ms(uint8_t rawMask, LINE_FOLLOW_Output *output)
{
    uint8_t activeMask = (uint8_t)(rawMask & LINE_FOLLOW_ACTIVE_MASK);
    float errorMm;

    rawMask = activeMask;
    if (activeMask == 0U) {
        if (gLineFollow.lostLineTicks < 255U) {
            ++gLineFollow.lostLineTicks;
        }

        if ((gLineFollow.lostLineTicks >= LINE_FOLLOW_LOST_STOP_TICKS) ||
            (gLineFollow.hasLastValidError == 0U)) {
            lineFollowWriteStoppedOutput(
                rawMask, activeMask,
                (gLineFollow.lostLineTicks >= LINE_FOLLOW_LOST_STOP_TICKS) ?
                    LINE_FOLLOW_STATE_LOST : LINE_FOLLOW_STATE_HOLD_LAST);
        } else {
            lineFollowRunPid(rawMask, activeMask,
                gLineFollow.lastValidErrorMm,
                LINE_FOLLOW_STATE_HOLD_LAST);
        }
    } else if (activeMask == LINE_FOLLOW_ACTIVE_MASK) {
        gLineFollow.lostLineTicks = 0U;
        gLineFollow.lastValidErrorMm = 0.0f;
        gLineFollow.hasLastValidError = 1U;
        lineFollowResetPidMemory();
        lineFollowWriteStraightOutput(
            rawMask, activeMask, LINE_FOLLOW_STATE_INTERSECTION);
    } else {
        if (gLineFollow.lostLineTicks != 0U) {
            lineFollowResetPidMemory();
        }
        gLineFollow.lostLineTicks = 0U;
        errorMm = lineFollowPositionError(activeMask);
        gLineFollow.lastValidErrorMm = errorMm;
        gLineFollow.hasLastValidError = 1U;
        lineFollowRunPid(
            rawMask, activeMask, errorMm, LINE_FOLLOW_STATE_TRACKING);
    }

    LINE_FOLLOW_getOutput(output);
}

void LINE_FOLLOW_getOutput(LINE_FOLLOW_Output *output)
{
    if (output != 0) {
        *output = gLineFollow.output;
    }
}
