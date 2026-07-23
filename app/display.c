#include "display.h"

#include "bsp/debug_uart.h"
#include "bsp/oled.h"

#define DISPLAY_OLED_LINE_COUNT      (4U)
#define DISPLAY_OLED_COLUMNS         (16U)
#define DISPLAY_OLED_PAGE_CHARACTERS \
    (DISPLAY_OLED_LINE_COUNT * DISPLAY_OLED_COLUMNS)

static uint16_t gLastDecodedKdX100[2];
static char gOledRunPage[DISPLAY_OLED_PAGE_CHARACTERS];
static uint8_t gOledRunPageIndex = DISPLAY_OLED_PAGE_CHARACTERS;

static void displayOledCopyText(
    uint8_t line, uint8_t column, const char *text)
{
    uint8_t index = (uint8_t)(
        ((line - 1U) * DISPLAY_OLED_COLUMNS) + column - 1U);
    uint8_t lineEnd = (uint8_t)(line * DISPLAY_OLED_COLUMNS);

    while ((*text != '\0') && (index < lineEnd)) {
        gOledRunPage[index] = *text;
        ++index;
        ++text;
    }
}

static void displayWriteSource(DISPLAY_Source source)
{
    if (source == DISPLAY_SOURCE_BLUETOOTH) {
        DEBUG_UART_writeString("[BT ");
    } else {
        DEBUG_UART_writeString("[DBG ");
    }
}

static void displayWriteFixed2(uint16_t valueX100)
{
    uint16_t fraction = (uint16_t)(valueX100 % 100U);

    DEBUG_UART_writeUInt32((uint32_t)(valueX100 / 100U));
    DEBUG_UART_writeByte((uint8_t)'.');
    DEBUG_UART_writeByte((uint8_t)('0' + (fraction / 10U)));
    DEBUG_UART_writeByte((uint8_t)('0' + (fraction % 10U)));
}

void DISPLAY_reportReceivedLine(DISPLAY_Source source, const char *line)
{
    displayWriteSource(source);
    DEBUG_UART_writeString("RX] ");
    DEBUG_UART_writeString(line);
    DEBUG_UART_writeString("\r\n");
}

void DISPLAY_reportPidDecoded(
    DISPLAY_Source source, uint16_t kpX100, uint16_t kiX100,
    uint16_t kdX100)
{
    if (source == DISPLAY_SOURCE_BLUETOOTH) {
        gLastDecodedKdX100[DISPLAY_SOURCE_BLUETOOTH] = kdX100;
    } else {
        gLastDecodedKdX100[DISPLAY_SOURCE_DEBUG_UART] = kdX100;
    }

    displayWriteSource(source);
    DEBUG_UART_writeString("PARSED] P=");
    displayWriteFixed2(kpX100);
    DEBUG_UART_writeString(" I=");
    displayWriteFixed2(kiX100);
    DEBUG_UART_writeString(" D=");
    displayWriteFixed2(kdX100);
    DEBUG_UART_writeString("\r\n");
}

void DISPLAY_reportPidApplied(
    DISPLAY_Source source, uint16_t kpX100, uint16_t kiX100)
{
    displayWriteSource(source);
    DEBUG_UART_writeString("PID] APPLIED P=");
    displayWriteFixed2(kpX100);
    DEBUG_UART_writeString(" I=");
    displayWriteFixed2(kiX100);
    DEBUG_UART_writeString(" D=0.00");
    if (((source == DISPLAY_SOURCE_BLUETOOTH) &&
            (gLastDecodedKdX100[DISPLAY_SOURCE_BLUETOOTH] != 0U)) ||
        ((source == DISPLAY_SOURCE_DEBUG_UART) &&
            (gLastDecodedKdX100[DISPLAY_SOURCE_DEBUG_UART] != 0U))) {
        DEBUG_UART_writeString(" (D IGNORED)");
    }
    DEBUG_UART_writeString("\r\n");
}

void DISPLAY_reportCommandRejected(DISPLAY_Source source, const char *reason)
{
    displayWriteSource(source);
    DEBUG_UART_writeString("PID] REJECTED: ");
    DEBUG_UART_writeString(reason);
    DEBUG_UART_writeString("\r\n");
}

void DISPLAY_oledBeginRunPage(void)
{
    uint8_t index;

    for (index = 0U; index < DISPLAY_OLED_PAGE_CHARACTERS; ++index) {
        gOledRunPage[index] = ' ';
    }
    displayOledCopyText(1U, 1U, "SP:");
    displayOledCopyText(1U, 10U, "V:");
    displayOledCopyText(2U, 1U, "PWM:");
    displayOledCopyText(3U, 1U, "P:");
    displayOledCopyText(3U, 6U, ".");
    displayOledCopyText(4U, 1U, "I:");
    displayOledCopyText(4U, 6U, ".");
    displayOledCopyText(4U, 10U, "D:0");
    gOledRunPageIndex = 0U;
}

void DISPLAY_oledService(void)
{
    uint8_t index;

    if (gOledRunPageIndex >= DISPLAY_OLED_PAGE_CHARACTERS) {
        return;
    }

    index = gOledRunPageIndex;
    OLED_ShowChar(
        (uint8_t)(index / DISPLAY_OLED_COLUMNS + 1U),
        (uint8_t)(index % DISPLAY_OLED_COLUMNS + 1U),
        gOledRunPage[index]);
    ++gOledRunPageIndex;
}

uint8_t DISPLAY_oledRunPageReady(void)
{
    return (uint8_t)(
        gOledRunPageIndex >= DISPLAY_OLED_PAGE_CHARACTERS);
}
