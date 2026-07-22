#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "bt_uart.h"
#include "encoder.h"
#include "oled.h"
#include "pi_tuner.h"
#include "wheel_speed.h"

volatile uint32_t gTick10ms;

#define DISPLAY_DYNAMIC_CHAR_COUNT (24U)

static const uint8_t gDisplayLines[DISPLAY_DYNAMIC_CHAR_COUNT] = {
    1U, 1U, 1U, 1U, 1U,
    1U, 1U, 1U, 1U,
    2U, 2U, 2U, 2U, 2U,
    3U, 3U, 3U, 3U, 3U,
    4U, 4U, 4U, 4U, 4U,
};

static const uint8_t gDisplayColumns[DISPLAY_DYNAMIC_CHAR_COUNT] = {
    4U, 5U, 6U, 7U, 8U,
    12U, 13U, 14U, 15U,
    5U, 6U, 7U, 8U, 9U,
    3U, 4U, 5U, 7U, 8U,
    3U, 4U, 5U, 7U, 8U,
};

static char gDisplayCharacters[DISPLAY_DYNAMIC_CHAR_COUNT];
static uint8_t gDisplayCharacterIndex;

static uint32_t displayMagnitude(int32_t value)
{
    return (value < 0) ?
        ((uint32_t)(-(value + 1)) + 1U) : (uint32_t)value;
}

static void displayFormatSigned(
    char *destination, int32_t value, uint8_t digits)
{
    uint32_t magnitude = displayMagnitude(value);
    uint32_t divisor = 1U;

    destination[0] = (value < 0) ? '-' : '+';
    for (uint8_t i = 1U; i < digits; ++i) {
        divisor *= 10U;
    }
    for (uint8_t i = 0U; i < digits; ++i) {
        destination[i + 1U] =
            (char)('0' + (magnitude / divisor) % 10U);
        divisor /= 10U;
    }
}

static void displayFormatFixed2Digits(char *destination, uint16_t valueX100)
{
    uint16_t whole = (uint16_t)(valueX100 / 100U);
    uint16_t fraction = (uint16_t)(valueX100 % 100U);

    destination[0] = (char)('0' + (whole / 100U) % 10U);
    destination[1] = (char)('0' + (whole / 10U) % 10U);
    destination[2] = (char)('0' + whole % 10U);
    destination[3] = (char)('0' + fraction / 10U);
    destination[4] = (char)('0' + fraction % 10U);
}

static void displayBeginRightWheelRefresh(void)
{
    WHEEL_SPEED_Diagnostics diagnostics;

    WHEEL_SPEED_getDiagnostics(WHEEL_SPEED_WHEEL_RIGHT, &diagnostics);
    displayFormatSigned(&gDisplayCharacters[0],
        diagnostics.setpointMmps, 4U);
    displayFormatSigned(&gDisplayCharacters[5],
        diagnostics.inputMmps, 3U);
    displayFormatSigned(&gDisplayCharacters[9],
        diagnostics.outputPwm, 4U);
    displayFormatFixed2Digits(&gDisplayCharacters[14], diagnostics.kpX100);
    displayFormatFixed2Digits(&gDisplayCharacters[19], diagnostics.kiX100);
    gDisplayCharacterIndex = 0U;
}

static void displayUpdateOneCharacter(void)
{
    if (gDisplayCharacterIndex >= DISPLAY_DYNAMIC_CHAR_COUNT) {
        return;
    }

    OLED_ShowChar(gDisplayLines[gDisplayCharacterIndex],
        gDisplayColumns[gDisplayCharacterIndex],
        gDisplayCharacters[gDisplayCharacterIndex]);
    ++gDisplayCharacterIndex;
}

int main(void)
{
    uint32_t lastControlTick = 0U;
    uint32_t lastDisplayTick = 0U;
    uint32_t lastBtTestTick = 0U;

    SYSCFG_DL_init();
    ENCODER_init();
    PI_TUNER_init();
    BT_UART_writeString("BT_UART_TEST\r\n");

    OLED_Init();
    OLED_ShowString(1U, 1U, "SP:");
    OLED_ShowString(1U, 10U, "V:");
    OLED_ShowString(2U, 1U, "PWM:");
    OLED_ShowString(3U, 1U, "P:");
    OLED_ShowChar(3U, 6U, '.');
    OLED_ShowString(4U, 1U, "I:");
    OLED_ShowChar(4U, 6U, '.');
    OLED_ShowString(4U, 10U, "D:0");
    displayBeginRightWheelRefresh();
    while (gDisplayCharacterIndex < DISPLAY_DYNAMIC_CHAR_COUNT) {
        displayUpdateOneCharacter();
    }

    gTick10ms = 0U;
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
    DL_TimerG_startCounter(TIMER_0_INST);

    while (1) {
        uint32_t now = gTick10ms;

        PI_TUNER_pollUart();

        if (now != lastControlTick) {
            lastControlTick = now;
            PI_TUNER_update10ms(now);
            if ((uint32_t)(now - lastDisplayTick) >= 50U) {
                lastDisplayTick = now;
                displayBeginRightWheelRefresh();
            }
            if ((uint32_t)(now - lastBtTestTick) >= 100U) {
                lastBtTestTick = now;
                BT_UART_writeString("BT_UART_TEST\r\n");
            }
            if ((now & 1U) == 0U) {
                displayUpdateOneCharacter();
            }
        }
    }
}

void TIMER_0_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(TIMER_0_INST)) {
        case DL_TIMER_IIDX_ZERO:
            ENCODER_updateSpeed10ms();
            ++gTick10ms;
            break;
        default:
            break;
    }
}
