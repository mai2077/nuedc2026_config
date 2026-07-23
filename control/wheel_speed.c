#include "wheel_speed.h"

#define WHEEL_SPEED_Q8_SCALE (256)

typedef struct {
    int16_t targetMmps;
    int16_t outputPwm;
    uint16_t kpX100;
    uint16_t kiX100;
    int32_t inputMmps;
    int32_t errorMmps;
    int32_t proportionalPwm;
    int32_t integralPwm;
    int32_t feedForwardPwm;
    int32_t derivativePwm;
} WheelSpeedPi;

static WheelSpeedPi gRightController;
static WheelSpeedPi gLeftController;

static int32_t wheelSpeedRoundDivide(int64_t numerator, int32_t denominator)
{
    if (numerator >= 0) {
        return (int32_t)((numerator + (denominator / 2)) / denominator);
    }
    return -(int32_t)((-numerator + (denominator / 2)) / denominator);
}

static int32_t wheelSpeedClamp(int32_t value, int32_t lower, int32_t upper)
{
    if (value > upper) {
        return upper;
    }
    if (value < lower) {
        return lower;
    }
    return value;
}

static int16_t wheelSpeedClampTarget(int16_t targetMmps)
{
    return (int16_t)wheelSpeedClamp(targetMmps,
        -WHEEL_SPEED_MAX_TARGET_MMPS, WHEEL_SPEED_MAX_TARGET_MMPS);
}

static void wheelSpeedResetPi(WheelSpeedPi *controller)
{
    controller->outputPwm = 0;
    controller->proportionalPwm = 0;
    controller->integralPwm = 0;
    controller->feedForwardPwm = 0;
    controller->derivativePwm = 0;
}

static WheelSpeedPi *wheelSpeedController(WHEEL_SPEED_Wheel wheel)
{
    return (wheel == WHEEL_SPEED_WHEEL_LEFT) ?
        &gLeftController : &gRightController;
}

static void wheelSpeedSetTarget(WheelSpeedPi *controller, int16_t targetMmps)
{
    int16_t target = wheelSpeedClampTarget(targetMmps);
    int16_t previous = controller->targetMmps;

    if ((target == 0) ||
        ((target > 0) && (previous <= 0)) ||
        ((target < 0) && (previous >= 0))) {
        wheelSpeedResetPi(controller);
    }
    controller->targetMmps = target;
}

static int16_t wheelSpeedUpdatePi(
    WheelSpeedPi *controller, int32_t measuredCounts10ms)
{
    int32_t errorQ8;
    int32_t feedForward;
    int32_t proportional;
    int32_t integralStep;
    int32_t candidateIntegral;
    int32_t candidateOutput;
    int32_t limitedOutput;

    controller->inputMmps =
        WHEEL_SPEED_counts10msToMmps(measuredCounts10ms);
    controller->errorMmps =
        (int32_t)controller->targetMmps - controller->inputMmps;

    if (controller->targetMmps == 0) {
        wheelSpeedResetPi(controller);
        return 0;
    }

    errorQ8 = WHEEL_SPEED_mmpsToCountsQ8(controller->targetMmps) -
              (measuredCounts10ms * WHEEL_SPEED_Q8_SCALE);
    proportional = wheelSpeedRoundDivide(
        (int64_t)errorQ8 * controller->kpX100,
        WHEEL_SPEED_Q8_SCALE * 100);
    feedForward = (controller->targetMmps > 0) ?
        WHEEL_SPEED_BASE_FEEDFORWARD_PWM :
        -WHEEL_SPEED_BASE_FEEDFORWARD_PWM;
    integralStep = wheelSpeedRoundDivide(
        (int64_t)errorQ8 * controller->kiX100,
        WHEEL_SPEED_Q8_SCALE * 100);
    candidateIntegral = wheelSpeedClamp(
        controller->integralPwm + integralStep,
        -WHEEL_SPEED_PWM_LIMIT, WHEEL_SPEED_PWM_LIMIT);
    candidateOutput = feedForward + proportional + candidateIntegral;
    limitedOutput = wheelSpeedClamp(candidateOutput,
        -WHEEL_SPEED_PWM_LIMIT, WHEEL_SPEED_PWM_LIMIT);

    if ((candidateOutput == limitedOutput) ||
        ((limitedOutput == WHEEL_SPEED_PWM_LIMIT) && (errorQ8 < 0)) ||
        ((limitedOutput == -WHEEL_SPEED_PWM_LIMIT) && (errorQ8 > 0))) {
        controller->integralPwm = candidateIntegral;
    }

    controller->proportionalPwm = proportional;
    controller->feedForwardPwm = feedForward;
    controller->derivativePwm = 0;
    controller->outputPwm = (int16_t)limitedOutput;
    return controller->outputPwm;
}

void WHEEL_SPEED_init(void)
{
    gRightController.targetMmps = 0;
    gRightController.kpX100 = WHEEL_SPEED_RIGHT_KP_X100;
    gRightController.kiX100 = WHEEL_SPEED_RIGHT_KI_X100;
    gRightController.inputMmps = 0;
    gRightController.errorMmps = 0;
    wheelSpeedResetPi(&gRightController);

    gLeftController.targetMmps = 0;
    gLeftController.kpX100 = WHEEL_SPEED_LEFT_KP_X100;
    gLeftController.kiX100 = WHEEL_SPEED_LEFT_KI_X100;
    gLeftController.inputMmps = 0;
    gLeftController.errorMmps = 0;
    wheelSpeedResetPi(&gLeftController);
}

void WHEEL_SPEED_setTargetsMmps(int16_t rightMmps, int16_t leftMmps)
{
    wheelSpeedSetTarget(&gRightController, rightMmps);
    wheelSpeedSetTarget(&gLeftController, leftMmps);
}

void WHEEL_SPEED_update10ms(
    int32_t rightCounts10ms, int32_t leftCounts10ms)
{
    (void)wheelSpeedUpdatePi(&gRightController, rightCounts10ms);
    (void)wheelSpeedUpdatePi(&gLeftController, leftCounts10ms);
}

uint8_t WHEEL_SPEED_setGainsX100(
    WHEEL_SPEED_Wheel wheel, uint16_t kpX100, uint16_t kiX100)
{
    WheelSpeedPi *controller;

    if (((wheel != WHEEL_SPEED_WHEEL_RIGHT) &&
         (wheel != WHEEL_SPEED_WHEEL_LEFT)) ||
        (kpX100 > WHEEL_SPEED_KP_X100_MAX) ||
        (kiX100 > WHEEL_SPEED_KI_X100_MAX)) {
        return 0U;
    }

    controller = wheelSpeedController(wheel);
    if ((controller->kpX100 == kpX100) &&
        (controller->kiX100 == kiX100)) {
        return 1U;
    }
    controller->kpX100 = kpX100;
    controller->kiX100 = kiX100;
    wheelSpeedResetPi(controller);
    return 1U;
}

void WHEEL_SPEED_getDiagnostics(
    WHEEL_SPEED_Wheel wheel, WHEEL_SPEED_Diagnostics *diagnostics)
{
    WheelSpeedPi *controller;

    if ((diagnostics == 0) ||
        ((wheel != WHEEL_SPEED_WHEEL_RIGHT) &&
         (wheel != WHEEL_SPEED_WHEEL_LEFT))) {
        return;
    }

    controller = wheelSpeedController(wheel);
    diagnostics->setpointMmps = controller->targetMmps;
    diagnostics->inputMmps = controller->inputMmps;
    diagnostics->outputPwm = controller->outputPwm;
    diagnostics->errorMmps = controller->errorMmps;
    diagnostics->proportionalPwm = controller->proportionalPwm;
    diagnostics->integralPwm = controller->integralPwm;
    diagnostics->feedForwardPwm = controller->feedForwardPwm;
    diagnostics->derivativePwm = controller->derivativePwm;
    diagnostics->kpX100 = controller->kpX100;
    diagnostics->kiX100 = controller->kiX100;
}

int16_t WHEEL_SPEED_getRightTargetMmps(void)
{
    return gRightController.targetMmps;
}

int16_t WHEEL_SPEED_getLeftTargetMmps(void)
{
    return gLeftController.targetMmps;
}

int16_t WHEEL_SPEED_getRightPwm(void)
{
    return gRightController.outputPwm;
}

int16_t WHEEL_SPEED_getLeftPwm(void)
{
    return gLeftController.outputPwm;
}

int32_t WHEEL_SPEED_mmpsToCountsQ8(int32_t speedMmps)
{
    return wheelSpeedRoundDivide(
        (int64_t)speedMmps * WHEEL_SPEED_COUNTS_PER_REV *
            WHEEL_SPEED_Q8_SCALE * 10,
        WHEEL_SPEED_WHEEL_CIRCUMFERENCE_X1000_MM);
}

int32_t WHEEL_SPEED_counts10msToMmps(int32_t counts10ms)
{
    return wheelSpeedRoundDivide(
        (int64_t)counts10ms * WHEEL_SPEED_WHEEL_CIRCUMFERENCE_X1000_MM,
        WHEEL_SPEED_COUNTS_PER_REV * 10);
}
