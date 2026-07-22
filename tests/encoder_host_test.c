#ifdef ENCODER_HOST_TEST

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "encoder.h"
#include "ti_msp_dl_config.h"

GPIO_Regs gTestGPIOA;
GPIO_Regs gTestGPIOB;
GPTIMER_Regs gTestTIMA0;
GPTIMER_Regs gTestTIMG6;

static uint32_t gDirectionPins;
static uint32_t gEncoder1Pending;
static uint32_t gEncoder2Pending;
static uint32_t gEnabledIrqs;

uint32_t DL_GPIO_readPins(GPIO_Regs *gpio, uint32_t pins)
{
    assert(gpio == ENCODER_DIR_PORT);
    return gDirectionPins & pins;
}

uint32_t DL_TimerA_getPendingInterrupt(GPTIMER_Regs *timer)
{
    assert(timer == ENCODER1_INST);
    uint32_t pending = gEncoder1Pending;
    gEncoder1Pending = 0U;
    return pending;
}

uint32_t DL_TimerG_getPendingInterrupt(GPTIMER_Regs *timer)
{
    assert(timer == ENCODER2_INST);
    uint32_t pending = gEncoder2Pending;
    gEncoder2Pending = 0U;
    return pending;
}

void NVIC_ClearPendingIRQ(int irqn)
{
    (void)irqn;
}

void NVIC_EnableIRQ(int irqn)
{
    if (irqn == ENCODER1_INST_INT_IRQN) {
        gEnabledIrqs |= 1U;
    } else if (irqn == ENCODER2_INST_INT_IRQN) {
        gEnabledIrqs |= 2U;
    } else {
        assert(0);
    }
}

void ENCODER1_INST_IRQHandler(void);
void ENCODER2_INST_IRQHandler(void);

static void pulseEncoder1(int directionHigh)
{
    if (directionHigh != 0) {
        gDirectionPins |= ENCODER_DIR_DIR1_PIN;
    } else {
        gDirectionPins &= ~ENCODER_DIR_DIR1_PIN;
    }
    gEncoder1Pending = DL_TIMER_IIDX_CC0_DN;
    ENCODER1_INST_IRQHandler();
}

static void pulseEncoder2(int directionHigh)
{
    if (directionHigh != 0) {
        gDirectionPins |= ENCODER_DIR_DIR2_PIN;
    } else {
        gDirectionPins &= ~ENCODER_DIR_DIR2_PIN;
    }
    gEncoder2Pending = DL_TIMER_IIDX_CC0_DN;
    ENCODER2_INST_IRQHandler();
}

static void testInitAndSignedPosition(void)
{
    gEnabledIrqs = 0U;
    ENCODER_init();

    assert(gEnabledIrqs == 3U);
    assert(ENCODER_getRightPosition() == 0);
    assert(ENCODER_getLeftPosition() == 0);

    pulseEncoder1(0);
    assert(ENCODER_getRightPosition() == 1);
    pulseEncoder1(1);
    assert(ENCODER_getRightPosition() == 0);

    pulseEncoder2(1);
    pulseEncoder2(1);
    assert(ENCODER_getLeftPosition() == 2);
    pulseEncoder2(0);
    assert(ENCODER_getLeftPosition() == 1);
}

static void testUnexpectedInterruptDoesNotCount(void)
{
    ENCODER_reset();
    gEncoder1Pending = 0U;
    ENCODER1_INST_IRQHandler();
    gEncoder2Pending = 0U;
    ENCODER2_INST_IRQHandler();
    assert(ENCODER_getRightPosition() == 0);
    assert(ENCODER_getLeftPosition() == 0);
}

static void testTenMillisecondSpeedDelta(void)
{
    ENCODER_reset();

    pulseEncoder1(0);
    pulseEncoder1(0);
    pulseEncoder1(0);
    pulseEncoder2(0);
    pulseEncoder2(0);
    ENCODER_updateSpeed10ms();

    assert(ENCODER_getRightPosition() == 3);
    assert(ENCODER_getLeftPosition() == -2);
    assert(ENCODER_getRightSpeed10ms() == 3);
    assert(ENCODER_getLeftSpeed10ms() == -2);

    ENCODER_updateSpeed10ms();
    assert(ENCODER_getRightSpeed10ms() == 0);
    assert(ENCODER_getLeftSpeed10ms() == 0);

    pulseEncoder1(1);
    pulseEncoder2(1);
    ENCODER_updateSpeed10ms();
    assert(ENCODER_getRightSpeed10ms() == -1);
    assert(ENCODER_getLeftSpeed10ms() == 1);
}

int main(void)
{
    testInitAndSignedPosition();
    testUnexpectedInterruptDoesNotCount();
    testTenMillisecondSpeedDelta();
    puts("encoder tests passed");
    return 0;
}

#endif /* ENCODER_HOST_TEST */
