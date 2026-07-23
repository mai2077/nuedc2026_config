#include "ti_msp_dl_config.h"
#include "debug_uart.h"
#include "track.h"

#define DEBUG_UART_RX_BUFFER_SIZE (128U)

static volatile uint8_t gDebugUartRxBuffer[DEBUG_UART_RX_BUFFER_SIZE];
static volatile uint8_t gDebugUartRxHead;
static volatile uint8_t gDebugUartRxTail;

static uint8_t DEBUG_UART_hexDigit(uint8_t value)
{
    static const char hexDigits[] = "0123456789ABCDEF";

    return (uint8_t)hexDigits[value & 0x0FU];
}

static uint32_t DEBUG_UART_magnitude(int32_t value)
{
    return (value < 0) ? ((uint32_t)(-(value + 1)) + 1U) : (uint32_t)value;
}

static uint8_t DEBUG_UART_nextRxIndex(uint8_t index)
{
    ++index;
    if (index >= DEBUG_UART_RX_BUFFER_SIZE) {
        index = 0U;
    }
    return index;
}

static void DEBUG_UART_storeReceivedByte(uint8_t data)
{
    uint8_t nextHead = DEBUG_UART_nextRxIndex(gDebugUartRxHead);

    if (nextHead == gDebugUartRxTail) {
        gDebugUartRxTail = DEBUG_UART_nextRxIndex(gDebugUartRxTail);
    }

    gDebugUartRxBuffer[gDebugUartRxHead] = data;
    gDebugUartRxHead = nextHead;
}

static uint8_t DEBUG_UART_popBufferedByte(uint8_t *data)
{
    if (gDebugUartRxTail == gDebugUartRxHead) {
        return 0U;
    }

    *data = gDebugUartRxBuffer[gDebugUartRxTail];
    gDebugUartRxTail = DEBUG_UART_nextRxIndex(gDebugUartRxTail);
    return 1U;
}

static void DEBUG_UART_drainRxFifo(void)
{
    while (DL_UART_Main_isRXFIFOEmpty(RESV1_UART_INST) == 0U) {
        DEBUG_UART_storeReceivedByte(
            DL_UART_Main_receiveData(RESV1_UART_INST));
    }
}

void DEBUG_UART_writeByte(uint8_t data)
{
    DL_UART_Main_transmitDataBlocking(RESV1_UART_INST, data);
}

void DEBUG_UART_writeString(const char *text)
{
    while (*text != '\0') {
        DEBUG_UART_writeByte((uint8_t)*text);
        ++text;
    }
}

void DEBUG_UART_writeUInt32(uint32_t value)
{
    uint8_t digits[10];
    uint8_t count = 0U;

    do {
        digits[count] = (uint8_t)(value % 10U);
        ++count;
        value /= 10U;
    } while (value != 0U);

    while (count > 0U) {
        --count;
        DEBUG_UART_writeByte((uint8_t)('0' + digits[count]));
    }
}

void DEBUG_UART_writeInt32(int32_t value)
{
    if (value < 0) {
        DEBUG_UART_writeByte((uint8_t)'-');
    }

    DEBUG_UART_writeUInt32(DEBUG_UART_magnitude(value));
}

void DEBUG_UART_writeHexByte(uint8_t value)
{
    DEBUG_UART_writeByte(DEBUG_UART_hexDigit((uint8_t)(value >> 4)));
    DEBUG_UART_writeByte(DEBUG_UART_hexDigit(value));
}

void DEBUG_UART_writeSignedFixed3(int32_t milliValue)
{
    uint32_t magnitude;
    uint32_t fraction;

    if (milliValue < 0) {
        DEBUG_UART_writeByte((uint8_t)'-');
    }

    magnitude = DEBUG_UART_magnitude(milliValue);
    fraction = magnitude % 1000U;

    DEBUG_UART_writeUInt32(magnitude / 1000U);
    DEBUG_UART_writeByte((uint8_t)'.');
    DEBUG_UART_writeByte((uint8_t)('0' + (fraction / 100U)));
    DEBUG_UART_writeByte((uint8_t)('0' + ((fraction / 10U) % 10U)));
    DEBUG_UART_writeByte((uint8_t)('0' + (fraction % 10U)));
}

void DEBUG_UART_writeTrackFrame(uint8_t rawMask)
{
    uint8_t bitNumber;

    rawMask = (uint8_t)(rawMask & TRACK_MASK_ALL);

    DEBUG_UART_writeString("TRACK mask=0x");
    DEBUG_UART_writeByte(DEBUG_UART_hexDigit((uint8_t)(rawMask >> 4)));
    DEBUG_UART_writeByte(DEBUG_UART_hexDigit(rawMask));
    DEBUG_UART_writeString(" bits=");

    for (bitNumber = 5U; bitNumber > 0U; --bitNumber) {
        uint8_t bitValue = (uint8_t)((rawMask >> (bitNumber - 1U)) & 0x01U);
        DEBUG_UART_writeByte((uint8_t)('0' + bitValue));
    }

    DEBUG_UART_writeString("\r\n");
}

void DEBUG_UART_initRxInterrupt(void)
{
    gDebugUartRxHead = 0U;
    gDebugUartRxTail = 0U;
    DEBUG_UART_drainRxFifo();

    DL_UART_Main_setRXFIFOThreshold(
        RESV1_UART_INST, DL_UART_MAIN_RX_FIFO_LEVEL_ONE_ENTRY);
    DL_UART_Main_clearInterruptStatus(RESV1_UART_INST,
        DL_UART_MAIN_INTERRUPT_RX |
        DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR |
        DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR);
    DL_UART_Main_enableInterrupt(RESV1_UART_INST,
        DL_UART_MAIN_INTERRUPT_RX |
        DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR |
        DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR);
    NVIC_ClearPendingIRQ(RESV1_UART_INST_INT_IRQN);
    NVIC_EnableIRQ(RESV1_UART_INST_INT_IRQN);
}

uint8_t DEBUG_UART_tryReadByte(uint8_t *data)
{
    if (data == 0) {
        return 0U;
    }

    if (DEBUG_UART_popBufferedByte(data) != 0U) {
        return 1U;
    }

    if (DL_UART_Main_isRXFIFOEmpty(RESV1_UART_INST)) {
        return 0U;
    }

    *data = DL_UART_Main_receiveData(RESV1_UART_INST);
    return 1U;
}

void RESV1_UART_INST_IRQHandler(void)
{
    switch (DL_UART_Main_getPendingInterrupt(RESV1_UART_INST)) {
        case DL_UART_MAIN_IIDX_RX:
        case DL_UART_MAIN_IIDX_RX_TIMEOUT_ERROR:
        case DL_UART_MAIN_IIDX_OVERRUN_ERROR:
            DEBUG_UART_drainRxFifo();
            break;
        default:
            break;
    }
}
