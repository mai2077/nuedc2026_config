#include "ti_msp_dl_config.h"
#include "bt_uart.h"
#include "track.h"

static uint8_t BT_UART_hexDigit(uint8_t value)
{
    static const char hexDigits[] = "0123456789ABCDEF";

    return (uint8_t)hexDigits[value & 0x0FU];
}

void BT_UART_writeByte(uint8_t data)
{
    DL_UART_Main_transmitDataBlocking(BT_UART_INST, data);
}

void BT_UART_writeString(const char *text)
{
    while (*text != '\0') {
        BT_UART_writeByte((uint8_t)*text);
        ++text;
    }
}

void BT_UART_writeTrackFrame(uint8_t rawMask)
{
    uint8_t bitNumber;

    rawMask = (uint8_t)(rawMask & TRACK_MASK_ALL);

    BT_UART_writeString("TRACK mask=0x");
    BT_UART_writeByte(BT_UART_hexDigit((uint8_t)(rawMask >> 4)));
    BT_UART_writeByte(BT_UART_hexDigit(rawMask));
    BT_UART_writeString(" bits=");

    for (bitNumber = 5U; bitNumber > 0U; --bitNumber) {
        uint8_t bitValue =
            (uint8_t)((rawMask >> (bitNumber - 1U)) & 0x01U);
        BT_UART_writeByte((uint8_t)('0' + bitValue));
    }

    BT_UART_writeString("\r\n");
}
