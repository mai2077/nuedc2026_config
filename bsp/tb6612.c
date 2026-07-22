#include "ti_msp_dl_config.h"
#include "tb6612.h"

static int8_t gMotor1Direction;
static int8_t gMotor2Direction;

static int32_t TB6612_clampCommand(int16_t signedPwm)
{
    int32_t command = signedPwm;

    if (command > TB6612_PWM_MAX) {
        command = TB6612_PWM_MAX;
    } else if (command < -TB6612_PWM_MAX) {
        command = -TB6612_PWM_MAX;
    }

    return command;
}

static uint32_t TB6612_stopCompare(void)
{
    return DL_TimerA_getLoadValue(TB_PWM_INST);
}

static uint32_t TB6612_dutyToCompare(uint32_t magnitude)
{
    uint32_t load = TB6612_stopCompare();
    uint32_t period = load + 1U;
    uint32_t dutyCounts;

    if (magnitude == 0U) {
        return load;
    }

    dutyCounts = (magnitude * period + (TB6612_PWM_MAX / 2U)) /
                 TB6612_PWM_MAX;
    if (dutyCounts >= period) {
        return 0U;
    }
    return period - dutyCounts;
}

static void TB6612_setMotor(GPIO_Regs *in1Port, uint32_t in1Pin,
    GPIO_Regs *in2Port, uint32_t in2Pin, DL_TIMER_CC_INDEX captureCompareIndex,
    int16_t signedPwm, int8_t *lastDirection)
{
    int32_t command = TB6612_clampCommand(signedPwm);
    int8_t direction = (command > 0) ? 1 : ((command < 0) ? -1 : 0);
    uint32_t magnitude = (command < 0) ? (uint32_t)(-command) :
                                          (uint32_t)command;
    uint32_t stopCompare = TB6612_stopCompare();

    if (command == 0) {
        DL_TimerA_setCaptureCompareValue(
            TB_PWM_INST, stopCompare, captureCompareIndex);
        DL_GPIO_clearPins(in1Port, in1Pin);
        DL_GPIO_clearPins(in2Port, in2Pin);
        *lastDirection = 0;
        return;
    }

    if (direction != *lastDirection) {
        /* Remove drive before changing direction to avoid a direction glitch. */
        DL_TimerA_setCaptureCompareValue(
            TB_PWM_INST, stopCompare, captureCompareIndex);
        DL_GPIO_clearPins(in1Port, in1Pin);
        DL_GPIO_clearPins(in2Port, in2Pin);
        delay_cycles(stopCompare + 1U);

        if (direction > 0) {
            DL_GPIO_setPins(in1Port, in1Pin);
        } else {
            DL_GPIO_setPins(in2Port, in2Pin);
        }
        *lastDirection = direction;
    }

    DL_TimerA_setCaptureCompareValue(
        TB_PWM_INST, TB6612_dutyToCompare(magnitude), captureCompareIndex);
}

void TB6612_init(void)
{
    TB6612_stopAll();
}

void TB6612_setMotor1(int16_t signedPwm)
{
    TB6612_setMotor(TB_DIR_AIN1_PORT, TB_DIR_AIN1_PIN,
        TB_DIR_AIN2_PORT, TB_DIR_AIN2_PIN, DL_TIMER_CC_0_INDEX, signedPwm,
        &gMotor1Direction);
}

void TB6612_setMotor2(int16_t signedPwm)
{
    TB6612_setMotor(TB_DIR_BIN1_PORT, TB_DIR_BIN1_PIN,
        TB_DIR_BIN2_PORT, TB_DIR_BIN2_PIN, DL_TIMER_CC_1_INDEX, signedPwm,
        &gMotor2Direction);
}

void TB6612_stopAll(void)
{
    uint32_t stopCompare = TB6612_stopCompare();

    DL_TimerA_setCaptureCompareValue(
        TB_PWM_INST, stopCompare, DL_TIMER_CC_0_INDEX);
    DL_TimerA_setCaptureCompareValue(
        TB_PWM_INST, stopCompare, DL_TIMER_CC_1_INDEX);

    DL_GPIO_clearPins(TB_DIR_AIN1_PORT, TB_DIR_AIN1_PIN);
    DL_GPIO_clearPins(TB_DIR_AIN2_PORT, TB_DIR_AIN2_PIN);
    DL_GPIO_clearPins(TB_DIR_BIN1_PORT, TB_DIR_BIN1_PIN);
    DL_GPIO_clearPins(TB_DIR_BIN2_PORT, TB_DIR_BIN2_PIN);
    gMotor1Direction = 0;
    gMotor2Direction = 0;
}
