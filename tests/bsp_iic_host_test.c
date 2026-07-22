#ifdef BSP_IIC_HOST_TEST

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ti_msp_dl_config.h"
#include "bsp_iic.h"

typedef enum
{
    TEST_I2C_NORMAL = 0,
    TEST_I2C_ERROR,
    TEST_I2C_TIMEOUT,
    TEST_I2C_SLOW_RX
} TestI2CMode;

I2C_Regs gTestI2C0;

static uint8_t gRegisters[4][128];
static uint8_t gCurrentBank;
static uint8_t gRegisterPointer;
static uint8_t gTxFifo[32];
static uint16_t gTxCount;
static uint8_t gRxFifo[32];
static uint16_t gRxCount;
static uint16_t gRxIndex;
static uint16_t gTransferCount;
static DL_I2C_CONTROLLER_STOP gAddressPhaseStop;
static DL_I2C_CONTROLLER_STOP gReadPhaseStop;
static TestI2CMode gMode;
static int gStickyError;
static int gWedged;
static unsigned int gRecoveryCount;
static unsigned int gStopRequestCount;
static unsigned int gRxWaitRemaining;

static void resetModel(void)
{
    memset(gRegisters, 0, sizeof(gRegisters));
    memset(gTxFifo, 0, sizeof(gTxFifo));
    memset(gRxFifo, 0, sizeof(gRxFifo));
    gRegisters[0][0x00U] = 0xEAU;
    gCurrentBank = 0U;
    gRegisterPointer = 0U;
    gTxCount = 0U;
    gRxCount = 0U;
    gRxIndex = 0U;
    gTransferCount = 0U;
    gAddressPhaseStop = DL_I2C_CONTROLLER_STOP_ENABLE;
    gReadPhaseStop = DL_I2C_CONTROLLER_STOP_DISABLE;
    gMode = TEST_I2C_NORMAL;
    gStickyError = 0;
    gWedged = 0;
    gRecoveryCount = 0U;
    gStopRequestCount = 0U;
    gRxWaitRemaining = 0U;
}

uint32_t DL_I2C_getControllerStatus(const I2C_Regs *i2c)
{
    assert(i2c == ICM_I2C_INST);

    if ((gMode == TEST_I2C_TIMEOUT) || (gWedged != 0)) {
        return 0U;
    }
    if (gStickyError != 0) {
        return DL_I2C_CONTROLLER_STATUS_IDLE |
               DL_I2C_CONTROLLER_STATUS_ERROR;
    }
    return DL_I2C_CONTROLLER_STATUS_IDLE;
}

void DL_I2C_flushControllerTXFIFO(I2C_Regs *i2c)
{
    assert(i2c == ICM_I2C_INST);
    gTxCount = 0U;
}

void DL_I2C_flushControllerRXFIFO(I2C_Regs *i2c)
{
    assert(i2c == ICM_I2C_INST);
    gRxCount = 0U;
    gRxIndex = 0U;
}

void DL_I2C_resetControllerTransfer(I2C_Regs *i2c)
{
    assert(i2c == ICM_I2C_INST);
    ++gRecoveryCount;
    gStickyError = 0;
    gWedged = 0;
}

void DL_I2C_enableStopCondition(I2C_Regs *i2c)
{
    assert(i2c == ICM_I2C_INST);
    ++gStopRequestCount;
}

void DL_I2C_clearInterruptStatus(I2C_Regs *i2c, uint32_t mask)
{
    assert(i2c == ICM_I2C_INST);
    (void)mask;
}

uint16_t DL_I2C_fillControllerTXFIFO(
    I2C_Regs *i2c, const uint8_t *buffer, uint16_t count)
{
    assert(i2c == ICM_I2C_INST);
    assert(count <= sizeof(gTxFifo));
    memcpy(gTxFifo, buffer, count);
    gTxCount = count;
    return count;
}

void DL_I2C_startControllerTransferAdvanced(I2C_Regs *i2c,
    uint32_t targetAddr, DL_I2C_CONTROLLER_DIRECTION direction,
    uint16_t length, DL_I2C_CONTROLLER_START start,
    DL_I2C_CONTROLLER_STOP stop, DL_I2C_CONTROLLER_ACK ack)
{
    uint16_t index;

    assert(i2c == ICM_I2C_INST);
    assert(targetAddr == 0x68U);
    assert(start == DL_I2C_CONTROLLER_START_ENABLE);
    assert(ack == DL_I2C_CONTROLLER_ACK_DISABLE);
    ++gTransferCount;

    if (gMode == TEST_I2C_ERROR) {
        gStickyError = 1;
        return;
    }
    if (gMode == TEST_I2C_TIMEOUT) {
        gWedged = 1;
        return;
    }

    if (direction == DL_I2C_CONTROLLER_DIRECTION_TX) {
        assert(length == gTxCount);
        assert(length > 0U);
        gRegisterPointer = gTxFifo[0];
        gAddressPhaseStop = stop;

        if (stop == DL_I2C_CONTROLLER_STOP_ENABLE) {
            for (index = 1U; index < length; ++index) {
                if (gRegisterPointer == 0x7FU) {
                    gCurrentBank =
                        (uint8_t)((gTxFifo[index] >> 4U) & 0x03U);
                } else {
                    gRegisters[gCurrentBank][gRegisterPointer++] =
                        gTxFifo[index];
                }
            }
        }
        gTxCount = 0U;
        return;
    }

    assert(stop == DL_I2C_CONTROLLER_STOP_ENABLE);
    assert(length <= sizeof(gRxFifo));
    gReadPhaseStop = stop;
    gRxWaitRemaining = (gMode == TEST_I2C_SLOW_RX) ? 2U : 0U;
    for (index = 0U; index < length; ++index) {
        gRxFifo[index] = gRegisters[gCurrentBank][gRegisterPointer++];
    }
    gRxCount = length;
    gRxIndex = 0U;
}

int DL_I2C_isControllerRXFIFOEmpty(const I2C_Regs *i2c)
{
    assert(i2c == ICM_I2C_INST);
    if ((gMode == TEST_I2C_TIMEOUT) || (gWedged != 0)) {
        return 1;
    }
    if (gRxWaitRemaining != 0U) {
        --gRxWaitRemaining;
        return 1;
    }
    return (gRxIndex >= gRxCount);
}

uint8_t DL_I2C_receiveControllerData(const I2C_Regs *i2c)
{
    assert(i2c == ICM_I2C_INST);
    assert(gRxIndex < gRxCount);
    gRxWaitRemaining = (gMode == TEST_I2C_SLOW_RX) ? 2U : 0U;
    return gRxFifo[gRxIndex++];
}

void delay_cycles(uint32_t cycles)
{
    (void)cycles;
}

static void testRegisterWriteAndRepeatedStartRead(void)
{
    uint8_t bank = 0x20U;
    uint8_t who = 0U;

    resetModel();
    assert(UserII2Dev.write_reg(0xD0U, 0x7FU, &bank, 1U, 100U) ==
           IIC_OK);
    assert(gCurrentBank == 2U);

    bank = 0x00U;
    assert(UserII2Dev.write_reg(0xD0U, 0x7FU, &bank, 1U, 100U) ==
           IIC_OK);
    gTransferCount = 0U;
    assert(UserII2Dev.read_reg(0xD0U, 0x00U, &who, 1U, 100U) ==
           IIC_OK);
    assert(who == 0xEAU);
    assert(gTransferCount == 2U);
    assert(gAddressPhaseStop == DL_I2C_CONTROLLER_STOP_DISABLE);
    assert(gReadPhaseStop == DL_I2C_CONTROLLER_STOP_ENABLE);
}

static void testErrorTimeoutAndReconnect(void)
{
    uint8_t who = 0U;

    resetModel();
    gMode = TEST_I2C_ERROR;
    assert(UserII2Dev.read_reg(0xD0U, 0x00U, &who, 1U, 100U) ==
           IIC_ERR);

    gMode = TEST_I2C_NORMAL;
    assert(UserII2Dev.read_reg(0xD0U, 0x00U, &who, 1U, 100U) ==
           IIC_OK);
    assert(who == 0xEAU);
    assert(gRecoveryCount > 0U);
    assert(gStopRequestCount > 0U);

    resetModel();
    gMode = TEST_I2C_TIMEOUT;
    assert(UserII2Dev.read_reg(0xD0U, 0x00U, &who, 1U, 1U) ==
           IIC_TIMEOUT);

    gMode = TEST_I2C_NORMAL;
    assert(UserII2Dev.read_reg(0xD0U, 0x00U, &who, 1U, 100U) ==
           IIC_OK);
}

static void testTransferBudgetAndFifoBound(void)
{
    uint8_t values[8] = {0};
    uint8_t readback[2] = {0};

    resetModel();
    gMode = TEST_I2C_SLOW_RX;
    assert(UserII2Dev.read_reg(0xD0U, 0x00U, readback, 2U, 2U) ==
           IIC_TIMEOUT);

    resetModel();
    assert(UserII2Dev.write_reg(0xD0U, 0x10U, values, 8U, 100U) ==
           IIC_ERR);
}

static void testInvalidArguments(void)
{
    uint8_t value = 0U;

    resetModel();
    assert(UserII2Dev.read_reg(0xD0U, 0x00U, NULL, 1U, 100U) ==
           IIC_ERR);
    assert(UserII2Dev.write_reg(0xD0U, 0x00U, NULL, 1U, 100U) ==
           IIC_ERR);
    assert(UserII2Dev.read_reg(0xD0U, 0x00U, &value, 0U, 100U) ==
           IIC_ERR);
}

int main(void)
{
    testRegisterWriteAndRepeatedStartRead();
    testErrorTimeoutAndReconnect();
    testTransferBudgetAndFifoBound();
    testInvalidArguments();
    puts("bsp iic tests passed");
    return 0;
}

#endif /* BSP_IIC_HOST_TEST */
