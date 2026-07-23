#include <stdint.h>

#include "ti_msp_dl_config.h"
#include "app/display.h"
#include "app/pi_tuner.h"
#include "bsp/debug_uart.h"
#include "bsp/encoder.h"
#include "bsp/oled.h"
#include "control/wheel_speed.h"

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

static void displayShowWaitPage(void)
{
    OLED_Clear();
    OLED_ShowString(1U, 1U, "WAIT KEY1");
    OLED_ShowString(2U, 1U, "CURVE BIAS:");
    OLED_ShowSignedNum(3U, 1U, PI_TUNER_getCurveBiasMmps(), 4U);
    OLED_ShowString(3U, 7U, "mm/s");
}

static void displayRefreshCurveBias(void)
{
    OLED_ShowSignedNum(3U, 1U, PI_TUNER_getCurveBiasMmps(), 4U);
}

int main(void)
{
    uint32_t lastControlTick = 0U;
    uint32_t lastDisplayTick = 0U;
    uint32_t lastCurveBiasRevision;

    SYSCFG_DL_init();
    DEBUG_UART_initRxInterrupt();
    ENCODER_init();
    PI_TUNER_init();

    OLED_Init();
    displayShowWaitPage();
    lastCurveBiasRevision = PI_TUNER_getCurveBiasRevision();

    gTick10ms = 0U;
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
    DL_TimerG_startCounter(TIMER_0_INST);

    while (1) {
        uint32_t now = gTick10ms;
        uint32_t curveBiasRevision;

        PI_TUNER_pollUart();
        curveBiasRevision = PI_TUNER_getCurveBiasRevision();
        if ((PI_TUNER_isRunning() == 0U) &&
            (curveBiasRevision != lastCurveBiasRevision)) {
            lastCurveBiasRevision = curveBiasRevision;
            displayRefreshCurveBias();
        }

        if (now != lastControlTick) {
            uint8_t wasRunning = PI_TUNER_isRunning();

            lastControlTick = now;
            PI_TUNER_update10ms(now);
            if ((wasRunning == 0U) && (PI_TUNER_isRunning() != 0U)) {
                DISPLAY_oledBeginRunPage();
                lastDisplayTick = now;
            }

            if (PI_TUNER_isRunning() != 0U) {
                if (DISPLAY_oledRunPageReady() == 0U) {
                    DISPLAY_oledService();
                } else {
                    if ((uint32_t)(now - lastDisplayTick) >= 50U) {
                        lastDisplayTick = now;
                        displayBeginRightWheelRefresh();
                    }
                    if ((now & 1U) == 0U) {
                        displayUpdateOneCharacter();
                    }
                }
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
