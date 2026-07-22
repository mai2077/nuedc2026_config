#ifdef ICM20948_HOST_TEST

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ti_msp_dl_config.h"
#include "icm20948.h"

#define TEST_ICM_ADDRESS       (0x68U)
#define TEST_REG_BANK_SEL      (0x7FU)
#define TEST_REG_WHO_AM_I      (0x00U)
#define TEST_REG_PWR_MGMT_1    (0x06U)
#define TEST_REG_PWR_MGMT_2    (0x07U)
#define TEST_REG_GYRO_XOUT_H   (0x33U)
#define TEST_REG_GYRO_DIV      (0x00U)
#define TEST_REG_GYRO_CONFIG_1 (0x01U)
#define TEST_MAG_ADDRESS       (0x0CU)
#define TEST_MAG_REG_WIA       (0x01U)

typedef enum {
    TEST_I2C_NORMAL = 0,
    TEST_I2C_ERROR,
    TEST_I2C_TIMEOUT
} TestI2CMode;

I2C_Regs gTestI2C0;
uint8_t gIcmRegisters[4][128];
uint8_t gMagRegisters[128];

static uint8_t gCurrentBank;
static uint8_t gRegisterPointer;
static uint8_t gTxFifo[8];
static uint16_t gTxCount;
static uint8_t gRxFifo[8];
static uint16_t gRxCount;
static uint16_t gRxIndex;
static TestI2CMode gI2CMode;
static int gStickyError;

static void resetI2CModel(void)
{
    memset(gIcmRegisters, 0, sizeof(gIcmRegisters));
    memset(gMagRegisters, 0, sizeof(gMagRegisters));
    gIcmRegisters[0][TEST_REG_WHO_AM_I] = 0xEAU;
    gMagRegisters[TEST_MAG_REG_WIA] = 0x09U;
    gCurrentBank = 0U;
    gRegisterPointer = 0U;
    gTxCount = 0U;
    gRxCount = 0U;
    gRxIndex = 0U;
    gI2CMode = TEST_I2C_NORMAL;
    gStickyError = 0;
}

uint32_t DL_I2C_getControllerStatus(const I2C_Regs *i2c)
{
    assert(i2c == ICM_I2C_INST);

    if (gI2CMode == TEST_I2C_TIMEOUT) {
        return 0U;
    }
    if (gStickyError) {
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
    gStickyError = 0;
}

void DL_I2C_enableStopCondition(I2C_Regs *i2c)
{
    assert(i2c == ICM_I2C_INST);
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
    assert((targetAddr == TEST_ICM_ADDRESS) ||
           (targetAddr == TEST_MAG_ADDRESS));
    assert(start == DL_I2C_CONTROLLER_START_ENABLE);
    assert(ack == DL_I2C_CONTROLLER_ACK_DISABLE);

    if (gI2CMode == TEST_I2C_ERROR) {
        gStickyError = 1;
        return;
    }
    if (gI2CMode == TEST_I2C_TIMEOUT) {
        return;
    }

    gStickyError = 0;

    if (direction == DL_I2C_CONTROLLER_DIRECTION_TX) {
        assert(length == gTxCount);
        assert(length > 0U);
        gRegisterPointer = gTxFifo[0];

        if (stop == DL_I2C_CONTROLLER_STOP_ENABLE) {
            for (index = 1U; index < length; ++index) {
                if ((targetAddr == TEST_ICM_ADDRESS) &&
                    (gRegisterPointer == TEST_REG_BANK_SEL)) {
                    gCurrentBank = (uint8_t)((gTxFifo[index] >> 4) & 0x03U);
                } else if (targetAddr == TEST_MAG_ADDRESS) {
                    gMagRegisters[gRegisterPointer] = gTxFifo[index];
                    ++gRegisterPointer;
                } else {
                    gIcmRegisters[gCurrentBank][gRegisterPointer] =
                        gTxFifo[index];
                    ++gRegisterPointer;
                }
            }
        }
        gTxCount = 0U;
        return;
    }

    assert(stop == DL_I2C_CONTROLLER_STOP_ENABLE);
    assert(length <= sizeof(gRxFifo));
    for (index = 0U; index < length; ++index) {
        gRxFifo[index] = (targetAddr == TEST_MAG_ADDRESS) ?
            gMagRegisters[gRegisterPointer] :
            gIcmRegisters[gCurrentBank][gRegisterPointer];
        ++gRegisterPointer;
    }
    gRxCount = length;
    gRxIndex = 0U;
}

int DL_I2C_isControllerRXFIFOEmpty(const I2C_Regs *i2c)
{
    assert(i2c == ICM_I2C_INST);
    return (gI2CMode == TEST_I2C_TIMEOUT) || (gRxIndex >= gRxCount);
}

uint8_t DL_I2C_receiveControllerData(const I2C_Regs *i2c)
{
    assert(i2c == ICM_I2C_INST);
    assert(gRxIndex < gRxCount);
    return gRxFifo[gRxIndex++];
}

void delay_cycles(uint32_t cycles)
{
    (void)cycles;
}

static void testScaleConversion(void)
{
    assert(ICM20948_rawToMilliDps(16384) == 1000000);
    assert(ICM20948_rawToMilliDps(-16384) == -1000000);
    assert(ICM20948_rawToMilliDps(0) == 0);
}

static void testInitializationAndSelfCheck(void)
{
    uint8_t whoAmI = 0U;

    resetI2CModel();
    assert(ICM20948_init(NULL) == ICM20948_STATUS_INVALID_ARGUMENT);
    assert(ICM20948_init(&whoAmI) == ICM20948_STATUS_OK);
    assert(whoAmI == 0xEAU);
    assert(gIcmRegisters[0][TEST_REG_PWR_MGMT_1] == 0x01U);
    assert(gIcmRegisters[0][TEST_REG_PWR_MGMT_2] == 0x00U);
    assert(gIcmRegisters[2][TEST_REG_GYRO_DIV] == 0x04U);
    assert(gIcmRegisters[2][TEST_REG_GYRO_CONFIG_1] == 0x1FU);
    assert(ICM20948_selfCheck(&whoAmI) == ICM20948_STATUS_OK);
    assert(whoAmI == 0xEAU);

    gIcmRegisters[2][TEST_REG_GYRO_CONFIG_1] = 0x00U;
    assert(ICM20948_selfCheck(&whoAmI) ==
        ICM20948_STATUS_CONFIG_MISMATCH);

    gIcmRegisters[0][TEST_REG_WHO_AM_I] = 0x00U;
    assert(ICM20948_selfCheck(&whoAmI) ==
        ICM20948_STATUS_WHO_AM_I_MISMATCH);
}

static void testGyroRead(void)
{
    ICM20948_GyroSample sample;
    uint8_t index;
    static const uint8_t gyroBytes[6] = {
        0x40U, 0x00U, 0xC0U, 0x00U, 0x00U, 0x00U};

    resetI2CModel();
    for (index = 0U; index < sizeof(gyroBytes); ++index) {
        gIcmRegisters[0][TEST_REG_GYRO_XOUT_H + index] = gyroBytes[index];
    }

    assert(ICM20948_readGyro(NULL) == ICM20948_STATUS_INVALID_ARGUMENT);
    assert(ICM20948_readGyro(&sample) == ICM20948_STATUS_OK);
    assert(sample.rawX == 16384);
    assert(sample.rawY == -16384);
    assert(sample.rawZ == 0);
    assert(sample.milliDpsX == 1000000);
    assert(sample.milliDpsY == -1000000);
    assert(sample.milliDpsZ == 0);
}

static void testTransferErrors(void)
{
    uint8_t whoAmI;

    resetI2CModel();
    gI2CMode = TEST_I2C_ERROR;
    assert(ICM20948_init(&whoAmI) == ICM20948_STATUS_I2C_ERROR);

    resetI2CModel();
    gI2CMode = TEST_I2C_TIMEOUT;
    assert(ICM20948_init(&whoAmI) == ICM20948_STATUS_TIMEOUT);

    resetI2CModel();
    assert(ICM20948_init(&whoAmI) == ICM20948_STATUS_OK);
    gI2CMode = TEST_I2C_ERROR;
    assert(ICM20948_selfCheck(&whoAmI) == ICM20948_STATUS_I2C_ERROR);
    gI2CMode = TEST_I2C_NORMAL;
    assert(ICM20948_selfCheck(&whoAmI) == ICM20948_STATUS_OK);

    resetI2CModel();
    gI2CMode = TEST_I2C_TIMEOUT;
    assert(ICM20948_selfCheck(&whoAmI) == ICM20948_STATUS_TIMEOUT);
}

int main(void)
{
    testScaleConversion();
    testInitializationAndSelfCheck();
    testGyroRead();
    testTransferErrors();

    puts("icm20948 driver tests passed");
    return 0;
}

#endif /* ICM20948_HOST_TEST */
