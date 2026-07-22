#ifdef TB6612_MOTOR_HOST_TEST

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "ti_msp_dl_config.h"
#include "tb6612.h"

GPIO_Regs gTestGPIOA;
GPIO_Regs gTestGPIOB;
GPTIMER_Regs gTestTIMA1;

static uint32_t gGPIOAState;
static uint32_t gGPIOBState;
static uint32_t gCompare[2];
static uint32_t gLastDelayCycles;
static uint32_t gDelayCallCount;

static uint32_t *gpioState(GPIO_Regs *gpio)
{
    assert((gpio == &gTestGPIOA) || (gpio == &gTestGPIOB));
    return (gpio == &gTestGPIOA) ? &gGPIOAState : &gGPIOBState;
}

void DL_GPIO_setPins(GPIO_Regs *gpio, uint32_t pins)
{
    *gpioState(gpio) |= pins;
}

void DL_GPIO_clearPins(GPIO_Regs *gpio, uint32_t pins)
{
    *gpioState(gpio) &= ~pins;
}

uint32_t DL_TimerA_getLoadValue(const GPTIMER_Regs *timer)
{
    assert(timer == TB_PWM_INST);
    return 999U;
}

void DL_TimerA_setCaptureCompareValue(
    GPTIMER_Regs *timer, uint32_t value, DL_TIMER_CC_INDEX index)
{
    assert(timer == TB_PWM_INST);
    assert(index <= DL_TIMER_CC_1_INDEX);
    gCompare[index] = value;
}

void delay_cycles(uint32_t cycles)
{
    gLastDelayCycles = cycles;
    ++gDelayCallCount;
}

static void resetModel(void)
{
    gGPIOAState = UINT32_MAX;
    gGPIOBState = UINT32_MAX;
    gCompare[0] = UINT32_MAX;
    gCompare[1] = UINT32_MAX;
    gLastDelayCycles = 0U;
    gDelayCallCount = 0U;
}

static void testInitAndStopUseActualZeroDuty(void)
{
    resetModel();
    TB6612_init();

    assert(gCompare[0] == 999U);
    assert(gCompare[1] == 999U);
    assert((gGPIOAState & TB_DIR_AIN1_PIN) == 0U);
    assert((gGPIOBState & (TB_DIR_AIN2_PIN | TB_DIR_BIN1_PIN |
                          TB_DIR_BIN2_PIN)) == 0U);
}

static void testMotor1DirectionAndFifteenPercentDuty(void)
{
    resetModel();
    TB6612_init();
    TB6612_setMotor1(TB6612_TEST_DUTY);
    assert((gGPIOAState & TB_DIR_AIN1_PIN) != 0U);
    assert((gGPIOBState & TB_DIR_AIN2_PIN) == 0U);
    assert(gCompare[0] == 850U);
    assert(gLastDelayCycles == 1000U);
    assert(gDelayCallCount == 1U);

    TB6612_setMotor1(TB6612_TEST_DUTY * 2);
    assert(gCompare[0] == 700U);
    assert(gDelayCallCount == 1U);

    TB6612_setMotor1(-TB6612_TEST_DUTY);
    assert((gGPIOAState & TB_DIR_AIN1_PIN) == 0U);
    assert((gGPIOBState & TB_DIR_AIN2_PIN) != 0U);
    assert(gCompare[0] == 850U);
    assert(gDelayCallCount == 2U);
}

static void testMotor2ClampAndStop(void)
{
    resetModel();
    TB6612_init();
    TB6612_setMotor2(2000);
    assert((gGPIOBState & TB_DIR_BIN1_PIN) != 0U);
    assert((gGPIOBState & TB_DIR_BIN2_PIN) == 0U);
    assert(gCompare[1] == 0U);

    TB6612_stopAll();
    assert(gCompare[0] == 999U);
    assert(gCompare[1] == 999U);
    assert((gGPIOAState & TB_DIR_AIN1_PIN) == 0U);
    assert((gGPIOBState & (TB_DIR_AIN2_PIN | TB_DIR_BIN1_PIN |
                          TB_DIR_BIN2_PIN)) == 0U);
}

int main(void)
{
    testInitAndStopUseActualZeroDuty();
    testMotor1DirectionAndFifteenPercentDuty();
    testMotor2ClampAndStop();
    puts("tb6612 motor tests passed");
    return 0;
}

#endif /* TB6612_MOTOR_HOST_TEST */
