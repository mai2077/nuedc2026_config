#ifdef TRACK_DIAG_HOST_TEST

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ti_msp_dl_config.h"
#include "bt_uart.h"
#include "debug_uart.h"
#include "track.h"

GPIO_Regs gTestGPIOB;
UART_Regs gTestUART0;
UART_Regs gTestUART2;

static uint32_t gRawGPIOPins;
static char gDebugTxBuffer[64];
static size_t gDebugTxLength;
static char gBtTxBuffer[64];
static size_t gBtTxLength;
static uint8_t gDebugRxByte;
static uint8_t gDebugRxAvailable;

uint32_t DL_GPIO_readPins(GPIO_Regs *gpio, uint32_t pins)
{
    assert(gpio == TRACK3_PORT);
    return gRawGPIOPins & pins;
}

void DL_UART_Main_transmitDataBlocking(UART_Regs *uart, uint8_t data)
{
    char *txBuffer;
    size_t *txLength;

    if (uart == RESV1_UART_INST) {
        txBuffer = gDebugTxBuffer;
        txLength = &gDebugTxLength;
    } else {
        assert(uart == BT_UART_INST);
        txBuffer = gBtTxBuffer;
        txLength = &gBtTxLength;
    }

    assert(*txLength + 1U < sizeof(gDebugTxBuffer));
    txBuffer[(*txLength)++] = (char)data;
    txBuffer[*txLength] = '\0';
}

bool DL_UART_Main_isRXFIFOEmpty(const UART_Regs *uart)
{
    assert(uart == RESV1_UART_INST);
    return gDebugRxAvailable == 0U;
}

uint8_t DL_UART_Main_receiveData(const UART_Regs *uart)
{
    assert(uart == RESV1_UART_INST);
    assert(gDebugRxAvailable != 0U);
    gDebugRxAvailable = 0U;
    return gDebugRxByte;
}

static void resetTxBuffers(void)
{
    gDebugTxLength = 0U;
    gDebugTxBuffer[0] = '\0';
    gBtTxLength = 0U;
    gBtTxBuffer[0] = '\0';
}

static void testTrackMaskMapping(void)
{
    const uint32_t allPins = TRACK3_OUT1_PIN | TRACK3_OUT2_PIN |
        TRACK3_OUT3_PIN | TRACK3_OUT4_PIN | TRACK3_OUT5_PIN;

    assert(TRACK_packRawPins(TRACK3_OUT1_PIN) == 0x01U);
    assert(TRACK_packRawPins(TRACK3_OUT2_PIN) == 0x02U);
    assert(TRACK_packRawPins(TRACK3_OUT3_PIN) == 0x04U);
    assert(TRACK_packRawPins(TRACK3_OUT4_PIN) == 0x08U);
    assert(TRACK_packRawPins(TRACK3_OUT5_PIN) == 0x10U);
    assert(TRACK_packRawPins(allPins) == 0x1FU);

    gRawGPIOPins = TRACK3_OUT5_PIN | TRACK3_OUT2_PIN;
    assert(TRACK_readRawMask() == 0x12U);
}

static void testDebugUARTOutput(void)
{
    resetTxBuffers();
    DEBUG_UART_writeString("UART OK\r\n");
    assert(strcmp(gDebugTxBuffer, "UART OK\r\n") == 0);
    assert(gBtTxLength == 0U);

    resetTxBuffers();
    DEBUG_UART_writeTrackFrame(0x05U);
    assert(strcmp(gDebugTxBuffer,
        "TRACK mask=0x05 bits=00101\r\n") == 0);
    assert(gBtTxLength == 0U);

    resetTxBuffers();
    DEBUG_UART_writeTrackFrame(0xFFU);
    assert(strcmp(gDebugTxBuffer,
        "TRACK mask=0x1F bits=11111\r\n") == 0);
    assert(gBtTxLength == 0U);

    resetTxBuffers();
    DEBUG_UART_writeInt32(-2147483647L - 1L);
    assert(strcmp(gDebugTxBuffer, "-2147483648") == 0);
    assert(gBtTxLength == 0U);

    resetTxBuffers();
    DEBUG_UART_writeHexByte(0xEAU);
    assert(strcmp(gDebugTxBuffer, "EA") == 0);
    assert(gBtTxLength == 0U);

    resetTxBuffers();
    DEBUG_UART_writeSignedFixed3(-519);
    assert(strcmp(gDebugTxBuffer, "-0.519") == 0);
    assert(gBtTxLength == 0U);
}

static void testBTUARTOutput(void)
{
    resetTxBuffers();
    BT_UART_writeString("BT OK\r\n");
    assert(strcmp(gBtTxBuffer, "BT OK\r\n") == 0);
    assert(gDebugTxLength == 0U);

    resetTxBuffers();
    BT_UART_writeTrackFrame(0x12U);
    assert(strcmp(gBtTxBuffer,
        "TRACK mask=0x12 bits=10010\r\n") == 0);
    assert(gDebugTxLength == 0U);
}

static void testDebugUARTNonblockingInput(void)
{
    uint8_t data = 0U;

    gDebugRxAvailable = 0U;
    assert(DEBUG_UART_tryReadByte(&data) == 0U);

    gDebugRxByte = (uint8_t)'P';
    gDebugRxAvailable = 1U;
    assert(DEBUG_UART_tryReadByte(&data) != 0U);
    assert(data == (uint8_t)'P');
    assert(DEBUG_UART_tryReadByte(&data) == 0U);
    assert(DEBUG_UART_tryReadByte(0) == 0U);
}

int main(void)
{
    testTrackMaskMapping();
    testDebugUARTOutput();
    testBTUARTOutput();
    testDebugUARTNonblockingInput();

    puts("track/uart diagnostic tests passed");
    return 0;
}

#endif /* TRACK_DIAG_HOST_TEST */
