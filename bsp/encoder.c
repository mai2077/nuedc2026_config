#include "encoder.h"

#include "board_config.h"
#include "ti_msp_dl_config.h"

static volatile uint32_t gRightPositionRaw;
static volatile uint32_t gLeftPositionRaw;
static volatile uint32_t gRightLastPositionRaw;
static volatile uint32_t gLeftLastPositionRaw;
static volatile int32_t gRightSpeed10ms;
static volatile int32_t gLeftSpeed10ms;

static void encoderRecordPulse(volatile uint32_t *position,
    uint32_t directionHigh, uint32_t highIsPositive)
{
    if ((directionHigh != 0U) == (highIsPositive != 0U)) {
        ++(*position);
    } else {
        --(*position);
    }
}

void ENCODER_reset(void)
{
    gRightPositionRaw = 0U;
    gLeftPositionRaw = 0U;
    gRightLastPositionRaw = 0U;
    gLeftLastPositionRaw = 0U;
    gRightSpeed10ms = 0;
    gLeftSpeed10ms = 0;
}

void ENCODER_init(void)
{
    ENCODER_reset();
    NVIC_ClearPendingIRQ(ENCODER1_INST_INT_IRQN);
    NVIC_ClearPendingIRQ(ENCODER2_INST_INT_IRQN);
    NVIC_EnableIRQ(ENCODER1_INST_INT_IRQN);
    NVIC_EnableIRQ(ENCODER2_INST_INT_IRQN);
}

void ENCODER_updateSpeed10ms(void)
{
    uint32_t rightPosition = gRightPositionRaw;
    uint32_t leftPosition = gLeftPositionRaw;

    gRightSpeed10ms = (int32_t)(rightPosition - gRightLastPositionRaw);
    gLeftSpeed10ms = (int32_t)(leftPosition - gLeftLastPositionRaw);
    gRightLastPositionRaw = rightPosition;
    gLeftLastPositionRaw = leftPosition;
}

int32_t ENCODER_getRightPosition(void)
{
    return (int32_t)gRightPositionRaw;
}

int32_t ENCODER_getLeftPosition(void)
{
    return (int32_t)gLeftPositionRaw;
}

int32_t ENCODER_getRightSpeed10ms(void)
{
    return gRightSpeed10ms;
}

int32_t ENCODER_getLeftSpeed10ms(void)
{
    return gLeftSpeed10ms;
}

void ENCODER1_INST_IRQHandler(void)
{
    switch (DL_TimerA_getPendingInterrupt(ENCODER1_INST)) {
        case DL_TIMER_IIDX_CC0_DN:
            encoderRecordPulse(&gRightPositionRaw,
                DL_GPIO_readPins(ENCODER_DIR_PORT, ENCODER_DIR_DIR1_PIN),
                ENCODER1_DIR_HIGH_IS_POSITIVE);
            break;
        default:
            break;
    }
}

void ENCODER2_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(ENCODER2_INST)) {
        case DL_TIMER_IIDX_CC0_DN:
            encoderRecordPulse(&gLeftPositionRaw,
                DL_GPIO_readPins(ENCODER_DIR_PORT, ENCODER_DIR_DIR2_PIN),
                ENCODER2_DIR_HIGH_IS_POSITIVE);
            break;
        default:
            break;
    }
}
