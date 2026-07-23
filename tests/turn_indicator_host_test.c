#ifdef TURN_INDICATOR_HOST_TEST

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "ti_msp_dl_config.h"
#include "bsp/turn_indicator.h"

GPIO_Regs gTestGPIOA;
GPIO_Regs gTestGPIOB;
static uint32_t gGPIOAState;
static uint32_t gGPIOBState;

static uint32_t *stateFor(GPIO_Regs *gpio)
{
    return (gpio == &gTestGPIOA) ? &gGPIOAState : &gGPIOBState;
}

uint32_t DL_GPIO_readPins(GPIO_Regs *gpio, uint32_t pins)
{
    return *stateFor(gpio) & pins;
}

void DL_GPIO_setPins(GPIO_Regs *gpio, uint32_t pins)
{
    *stateFor(gpio) |= pins;
}

void DL_GPIO_clearPins(GPIO_Regs *gpio, uint32_t pins)
{
    *stateFor(gpio) &= ~pins;
}

static void resetModel(void)
{
    gGPIOAState = 0U;
    gGPIOBState = 0U;
    TURN_INDICATOR_init();
}

static void testPulseDurations(void)
{
    uint16_t tick;

    resetModel();
    TURN_INDICATOR_start();
    assert((gGPIOBState & BUZZER_PIN_PIN) != 0U);
    assert((gGPIOAState & LED_LED1_PIN) != 0U);

    for (tick = 1U; tick < TURN_INDICATOR_BUZZER_TICKS; ++tick) {
        TURN_INDICATOR_update10ms();
        assert((gGPIOBState & BUZZER_PIN_PIN) != 0U);
    }
    TURN_INDICATOR_update10ms();
    assert((gGPIOBState & BUZZER_PIN_PIN) == 0U);
    assert((gGPIOAState & LED_LED1_PIN) != 0U);

    for (tick = TURN_INDICATOR_BUZZER_TICKS + 1U;
         tick < TURN_INDICATOR_LED_TICKS; ++tick) {
        TURN_INDICATOR_update10ms();
        assert((gGPIOAState & LED_LED1_PIN) != 0U);
    }
    TURN_INDICATOR_update10ms();
    assert((gGPIOAState & LED_LED1_PIN) == 0U);
}

static void testStopIsImmediateAndStartRestartsPulse(void)
{
    resetModel();
    TURN_INDICATOR_start();
    TURN_INDICATOR_update10ms();
    TURN_INDICATOR_stop();
    assert(gGPIOAState == 0U);
    assert(gGPIOBState == 0U);

    TURN_INDICATOR_start();
    assert((gGPIOBState & BUZZER_PIN_PIN) != 0U);
    assert((gGPIOAState & LED_LED1_PIN) != 0U);
}

int main(void)
{
    testPulseDurations();
    testStopIsImmediateAndStartRestartsPulse();
    puts("turn indicator tests passed");
    return 0;
}

#endif /* TURN_INDICATOR_HOST_TEST */

