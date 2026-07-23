#include "ti_msp_dl_config.h"
#include "bt_uart.h"
#include "track.h"

#define BT_UART_RX_BUFFER_SIZE (1024U)
#define BT_UART_TX_BUFFER_SIZE (512U)

static volatile uint8_t gBtUartRxBuffer[BT_UART_RX_BUFFER_SIZE];
static volatile uint16_t gBtUartRxHead;
static volatile uint16_t gBtUartRxTail;
static volatile uint16_t gBtUartRxOverflowCount;
static volatile uint8_t gBtUartTxBuffer[BT_UART_TX_BUFFER_SIZE];
static volatile uint16_t gBtUartTxHead;
static volatile uint16_t gBtUartTxTail;

static uint8_t BT_UART_hexDigit(uint8_t value)
{
    static const char hexDigits[] = "0123456789ABCDEF";

    return (uint8_t)hexDigits[value & 0x0FU];
}

static uint32_t BT_UART_magnitude(int32_t value)
{
    return (value < 0) ? ((uint32_t)(-(value + 1)) + 1U) : (uint32_t)value;
}

static uint16_t BT_UART_nextRxIndex(uint16_t index)
{
    ++index;
    if (index >= BT_UART_RX_BUFFER_SIZE) {
        index = 0U;
    }
    return index;
}

static void BT_UART_storeReceivedByte(uint8_t data)
{
    uint16_t nextHead = BT_UART_nextRxIndex(gBtUartRxHead);

    if (nextHead == gBtUartRxTail) {
        if (gBtUartRxOverflowCount < 65535U) {
            ++gBtUartRxOverflowCount;
        }
        gBtUartRxTail = BT_UART_nextRxIndex(gBtUartRxTail);
    }

    gBtUartRxBuffer[gBtUartRxHead] = data;
    gBtUartRxHead = nextHead;
}

static uint16_t BT_UART_nextTxIndex(uint16_t index)
{
    ++index;
    if (index >= BT_UART_TX_BUFFER_SIZE) {
        index = 0U;
    }
    return index;
}

static void BT_UART_storeTxByte(uint8_t data)
{
    uint16_t nextHead = BT_UART_nextTxIndex(gBtUartTxHead);

    if (nextHead == gBtUartTxTail) {
        gBtUartTxTail = BT_UART_nextTxIndex(gBtUartTxTail);
    }

    gBtUartTxBuffer[gBtUartTxHead] = data;
    gBtUartTxHead = nextHead;
}

static uint8_t BT_UART_popTxByte(uint8_t *data)
{
    if (gBtUartTxTail == gBtUartTxHead) {
        return 0U;
    }

    *data = gBtUartTxBuffer[gBtUartTxTail];
    gBtUartTxTail = BT_UART_nextTxIndex(gBtUartTxTail);
    return 1U;
}

static void BT_UART_fillTxFifo(void)
{
    uint8_t data;

    while (DL_UART_Main_isTXFIFOFull(BT_UART_INST) == 0U) {
        if (BT_UART_popTxByte(&data) == 0U) {
            DL_UART_Main_disableInterrupt(
                BT_UART_INST, DL_UART_MAIN_INTERRUPT_TX);
            return;
        }
        DL_UART_Main_transmitData(BT_UART_INST, data);
    }

    DL_UART_Main_enableInterrupt(BT_UART_INST, DL_UART_MAIN_INTERRUPT_TX);
}

static uint8_t BT_UART_popBufferedByte(uint8_t *data)
{
    if (gBtUartRxTail == gBtUartRxHead) {
        return 0U;
    }

    *data = gBtUartRxBuffer[gBtUartRxTail];
    gBtUartRxTail = BT_UART_nextRxIndex(gBtUartRxTail);
    return 1U;
}

static void BT_UART_drainRxFifo(void)
{
    while (DL_UART_Main_isRXFIFOEmpty(BT_UART_INST) == 0U) {
        BT_UART_storeReceivedByte(DL_UART_Main_receiveData(BT_UART_INST));
    }
}

void BT_UART_writeByte(uint8_t data)
{
    NVIC_DisableIRQ(BT_UART_INST_INT_IRQN);
    BT_UART_storeTxByte(data);
    BT_UART_fillTxFifo();
    NVIC_EnableIRQ(BT_UART_INST_INT_IRQN);
}

void BT_UART_writeString(const char *text)
{
    while (*text != '\0') {
        BT_UART_writeByte((uint8_t)*text);
        ++text;
    }
}

void BT_UART_writeUInt32(uint32_t value)
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
        BT_UART_writeByte((uint8_t)('0' + digits[count]));
    }
}

void BT_UART_writeInt32(int32_t value)
{
    if (value < 0) {
        BT_UART_writeByte((uint8_t)'-');
    }

    BT_UART_writeUInt32(BT_UART_magnitude(value));
}

void BT_UART_writeSignedFixed3(int32_t milliValue)
{
    uint32_t magnitude;
    uint32_t fraction;

    if (milliValue < 0) {
        BT_UART_writeByte((uint8_t)'-');
    }

    magnitude = BT_UART_magnitude(milliValue);
    fraction = magnitude % 1000U;

    BT_UART_writeUInt32(magnitude / 1000U);
    BT_UART_writeByte((uint8_t)'.');
    BT_UART_writeByte((uint8_t)('0' + (fraction / 100U)));
    BT_UART_writeByte((uint8_t)('0' + ((fraction / 10U) % 10U)));
    BT_UART_writeByte((uint8_t)('0' + (fraction % 10U)));
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

void BT_UART_initRxInterrupt(void)
{
    gBtUartRxHead = 0U;
    gBtUartRxTail = 0U;
    gBtUartRxOverflowCount = 0U;
    gBtUartTxHead = 0U;
    gBtUartTxTail = 0U;
    BT_UART_drainRxFifo();

    DL_UART_Main_setRXFIFOThreshold(
        BT_UART_INST, DL_UART_MAIN_RX_FIFO_LEVEL_ONE_ENTRY);
    DL_UART_Main_setTXFIFOThreshold(
        BT_UART_INST, DL_UART_MAIN_TX_FIFO_LEVEL_EMPTY);
    DL_UART_Main_clearInterruptStatus(BT_UART_INST,
        DL_UART_MAIN_INTERRUPT_RX |
        DL_UART_MAIN_INTERRUPT_TX |
        DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR |
        DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR);
    DL_UART_Main_disableInterrupt(BT_UART_INST, DL_UART_MAIN_INTERRUPT_TX);
    DL_UART_Main_enableInterrupt(BT_UART_INST,
        DL_UART_MAIN_INTERRUPT_RX |
        DL_UART_MAIN_INTERRUPT_RX_TIMEOUT_ERROR |
        DL_UART_MAIN_INTERRUPT_OVERRUN_ERROR);
    NVIC_ClearPendingIRQ(BT_UART_INST_INT_IRQN);
    NVIC_SetPriority(BT_UART_INST_INT_IRQN, 0);
    NVIC_EnableIRQ(BT_UART_INST_INT_IRQN);
}

uint8_t BT_UART_tryReadByte(uint8_t *data)
{
    if (data == 0) {
        return 0U;
    }

    if (BT_UART_popBufferedByte(data) != 0U) {
        return 1U;
    }

    if (DL_UART_Main_isRXFIFOEmpty(BT_UART_INST)) {
        return 0U;
    }

    *data = DL_UART_Main_receiveData(BT_UART_INST);
    return 1U;
}

uint16_t BT_UART_getAndClearRxOverflowCount(void)
{
    uint16_t count;

    NVIC_DisableIRQ(BT_UART_INST_INT_IRQN);
    count = gBtUartRxOverflowCount;
    gBtUartRxOverflowCount = 0U;
    NVIC_EnableIRQ(BT_UART_INST_INT_IRQN);
    return count;
}

void BT_UART_INST_IRQHandler(void)
{
    uint32_t pending;

    do {
        pending = DL_UART_Main_getPendingInterrupt(BT_UART_INST);
        switch (pending) {
            case DL_UART_MAIN_IIDX_TX:
                BT_UART_drainRxFifo();
                BT_UART_fillTxFifo();
                break;
            case DL_UART_MAIN_IIDX_RX:
            case DL_UART_MAIN_IIDX_RX_TIMEOUT_ERROR:
            case DL_UART_MAIN_IIDX_OVERRUN_ERROR:
                BT_UART_drainRxFifo();
                break;
            default:
                break;
        }
    } while (pending != DL_UART_MAIN_IIDX_NO_INTERRUPT);
}
