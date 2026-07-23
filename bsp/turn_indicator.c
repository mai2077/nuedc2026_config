#include "turn_indicator.h"

#include "ti_msp_dl_config.h"

static uint16_t gBuzzerTicks;
static uint16_t gLedTicks;

void TURN_INDICATOR_stop(void)
{
    gBuzzerTicks = 0U;
    gLedTicks = 0U;
    DL_GPIO_clearPins(BUZZER_PORT, BUZZER_PIN_PIN);
    DL_GPIO_clearPins(LED_PORT, LED_LED1_PIN);
}

void TURN_INDICATOR_init(void)
{
    TURN_INDICATOR_stop();
}

void TURN_INDICATOR_start(void)
{
    gBuzzerTicks = TURN_INDICATOR_BUZZER_TICKS;
    gLedTicks = TURN_INDICATOR_LED_TICKS;
    DL_GPIO_setPins(BUZZER_PORT, BUZZER_PIN_PIN);
    DL_GPIO_setPins(LED_PORT, LED_LED1_PIN);
}

void TURN_INDICATOR_update10ms(void)
{
    if (gBuzzerTicks != 0U) {
        --gBuzzerTicks;
        if (gBuzzerTicks == 0U) {
            DL_GPIO_clearPins(BUZZER_PORT, BUZZER_PIN_PIN);
        }
    }

    if (gLedTicks != 0U) {
        --gLedTicks;
        if (gLedTicks == 0U) {
            DL_GPIO_clearPins(LED_PORT, LED_LED1_PIN);
        }
    }
}

