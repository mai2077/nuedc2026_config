#include "display.h"

#include "bsp/debug_uart.h"

static uint16_t gLastDecodedKdX100[2];

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
