#ifdef DISPLAY_OLED_HOST_TEST

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "app/display.h"
#include "control/wheel_speed.h"

void DISPLAY_oledBeginRunPage(void);
void DISPLAY_oledService(void);
uint8_t DISPLAY_oledRunPageReady(void);

static uint16_t gOledCharacterWrites;

void DEBUG_UART_writeByte(uint8_t data) { (void)data; }
void DEBUG_UART_writeString(const char *text) { (void)text; }
void DEBUG_UART_writeUInt32(uint32_t value) { (void)value; }

void OLED_ShowChar(
    uint8_t line, uint8_t column, char character)
{
    (void)line;
    (void)column;
    (void)character;
    ++gOledCharacterWrites;
}

void WHEEL_SPEED_getDiagnostics(
    WHEEL_SPEED_Wheel wheel, WHEEL_SPEED_Diagnostics *diagnostics)
{
    (void)wheel;
    if (diagnostics != 0) {
        *diagnostics = (WHEEL_SPEED_Diagnostics){0};
    }
}

static void testRunPageStartsWithoutBlockingOledWrites(void)
{
    uint8_t character;

    gOledCharacterWrites = 0U;
    DISPLAY_oledBeginRunPage();
    assert(gOledCharacterWrites == 0U);
    assert(DISPLAY_oledRunPageReady() == 0U);

    DISPLAY_oledService();
    assert(gOledCharacterWrites == 1U);
    assert(DISPLAY_oledRunPageReady() == 0U);

    for (character = 1U; character < 64U; ++character) {
        DISPLAY_oledService();
    }
    assert(gOledCharacterWrites == 64U);
    assert(DISPLAY_oledRunPageReady() != 0U);
}

int main(void)
{
    testRunPageStartsWithoutBlockingOledWrites();
    puts("display OLED scheduling tests passed");
    return 0;
}

#endif /* DISPLAY_OLED_HOST_TEST */
