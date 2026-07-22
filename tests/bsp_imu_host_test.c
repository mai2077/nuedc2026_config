#ifdef BSP_IMU_HOST_TEST

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "bsp_imu.h"
#include "icm_20948_reg.h"

#define ICM_ADDRESS_8BIT (ICM20948_DEV << 1U)
#define MAG_ADDRESS_8BIT (AK09916_DEV << 1U)

static uint8_t gIcmRegisters[4][128];
static uint8_t gMagRegisters[128];
static uint8_t gCurrentBank;
static uint8_t gIicError;
static uint8_t gMagIicError;
static uint32_t gTickMs;

static IIC_Status_t fakeWrite(uint16_t DevAddress,
                              uint8_t *pData,
                              uint16_t Size,
                              unsigned long Timeout)
{
    (void)DevAddress;
    (void)pData;
    (void)Size;
    (void)Timeout;
    return IIC_ERR;
}

static IIC_Status_t fakeRead(uint16_t DevAddress,
                             uint8_t *pData,
                             uint16_t Size,
                             unsigned long Timeout)
{
    (void)DevAddress;
    (void)pData;
    (void)Size;
    (void)Timeout;
    return IIC_ERR;
}

static IIC_Status_t fakeWriteReg(uint16_t DevAddress,
                                 uint16_t MemAddress,
                                 uint8_t *pData,
                                 uint16_t Size,
                                 unsigned long Timeout)
{
    uint16_t index;

    (void)Timeout;
    if ((gIicError != 0U) ||
        ((gMagIicError != 0U) && (DevAddress == MAG_ADDRESS_8BIT)) ||
        (pData == NULL) || (Size == 0U) ||
        (MemAddress > 0xFFU)) {
        return IIC_ERR;
    }

    if (DevAddress == ICM_ADDRESS_8BIT) {
        if (MemAddress == REG_BANK_SEL) {
            gCurrentBank = (uint8_t)((pData[0] >> 4U) & 0x03U);
            return IIC_OK;
        }
        for (index = 0U; index < Size; ++index) {
            gIcmRegisters[gCurrentBank][MemAddress + index] = pData[index];
        }
        return IIC_OK;
    }

    if (DevAddress == MAG_ADDRESS_8BIT) {
        for (index = 0U; index < Size; ++index) {
            gMagRegisters[MemAddress + index] = pData[index];
        }
        return IIC_OK;
    }

    return IIC_ERR;
}

static IIC_Status_t fakeReadReg(uint16_t DevAddress,
                                uint16_t MemAddress,
                                uint8_t *pData,
                                uint16_t Size,
                                unsigned long Timeout)
{
    uint16_t index;

    (void)Timeout;
    if ((gIicError != 0U) ||
        ((gMagIicError != 0U) && (DevAddress == MAG_ADDRESS_8BIT)) ||
        (pData == NULL) || (Size == 0U) ||
        (MemAddress > 0xFFU)) {
        return IIC_ERR;
    }

    if (DevAddress == ICM_ADDRESS_8BIT) {
        for (index = 0U; index < Size; ++index) {
            pData[index] = gIcmRegisters[gCurrentBank][MemAddress + index];
        }
        return IIC_OK;
    }

    if (DevAddress == MAG_ADDRESS_8BIT) {
        for (index = 0U; index < Size; ++index) {
            pData[index] = gMagRegisters[MemAddress + index];
        }
        return IIC_OK;
    }

    return IIC_ERR;
}

static void fakeDelay(uint16_t ms)
{
    (void)ms;
}

IICInterface_t UserII2Dev = {
    .write = fakeWrite,
    .read = fakeRead,
    .write_reg = fakeWriteReg,
    .read_reg = fakeReadReg,
    .delay_ms = fakeDelay
};

uint32_t bsp_imu_port_get_tick_ms(void)
{
    return gTickMs;
}

static void putS16BE(uint8_t *target, int16_t value)
{
    uint16_t raw = (uint16_t)value;

    target[0] = (uint8_t)(raw >> 8U);
    target[1] = (uint8_t)raw;
}

static void putS16LE(uint8_t *target, int16_t value)
{
    uint16_t raw = (uint16_t)value;

    target[0] = (uint8_t)raw;
    target[1] = (uint8_t)(raw >> 8U);
}

static void resetModel(void)
{
    memset(gIcmRegisters, 0, sizeof(gIcmRegisters));
    memset(gMagRegisters, 0, sizeof(gMagRegisters));
    gCurrentBank = 0U;
    gIicError = 0U;
    gMagIicError = 0U;
    gTickMs = 0U;

    gIcmRegisters[0][WHO_AM_I] = 0xEAU;
    gMagRegisters[WIA] = 0x09U;
    putS16BE(&gIcmRegisters[0][ACCEL_XOUT_H], 16384);
    putS16BE(&gIcmRegisters[0][ACCEL_XOUT_H + 2U], 0);
    putS16BE(&gIcmRegisters[0][ACCEL_XOUT_H + 4U], 0);
    putS16BE(&gIcmRegisters[0][ACCEL_XOUT_H + 6U], 16384);
    putS16BE(&gIcmRegisters[0][ACCEL_XOUT_H + 8U], -16384);
    putS16BE(&gIcmRegisters[0][ACCEL_XOUT_H + 10U], 0);
    putS16LE(&gMagRegisters[HXL], 100);
    putS16LE(&gMagRegisters[HXL + 2U], -200);
    putS16LE(&gMagRegisters[HXL + 4U], 300);
    gMagRegisters[HXL + 6U] = 0U;
}

static void testInitializationAndSample(void)
{
    IMU_DATA_t sample = {0};
    bsp_imu_diag_t diag = {0};
    pIMUInterface_t imu;

    resetModel();
    bsp_imu_select_device(IMU_DEVICE_ICM20948);
    imu = bsp_imu_get_interface();
    assert(imu == &UserICM20948);
    assert(imu->Init() == 0U);
    assert(gIcmRegisters[0][PWR_MGMT_1] == 0x01U);
    assert(gIcmRegisters[0][PWR_MGMT_2] == 0x00U);
    assert(gIcmRegisters[2][GYRO_SMPLRT_DIV] == 0x04U);
    assert(gIcmRegisters[2][GYRO_CONFIG_1] == 0x1FU);
    assert(gIcmRegisters[2][ACCEL_SMPLRT_DIV_2] == 0x04U);
    assert(gIcmRegisters[2][ACCEL_CONFIG] == 0x29U);

    bsp_imu_get_diag(&diag);
    assert(diag.init_ok != 0U);
    assert(diag.init_stage == BSP_IMU_INIT_STAGE_READY);
    assert(diag.last_who_am_i == 0xEAU);
    assert(diag.mag_available != 0U);

    assert(bsp_imu_update_9axis_checked(&sample) == 0U);
    assert(fabsf(sample.accel.x - 9.8f) < 0.1f);
    assert(fabsf(sample.gyro.x - (1000.0f * 3.14159265f / 180.0f)) <
           0.01f);
    assert(fabsf(sample.gyro.y + (1000.0f * 3.14159265f / 180.0f)) <
           0.01f);
    assert(fabsf(sample.magn.x - 14.9536f) < 0.01f);
}

static void testFailedSampleDoesNotModifyOutput(void)
{
    IMU_DATA_t sample = {{1.0f, 2.0f, 3.0f},
                         {4.0f, 5.0f, 6.0f},
                         {7.0f, 8.0f, 9.0f}};
    IMU_DATA_t before = sample;

    gIicError = 1U;
    assert(bsp_imu_update_9axis_checked(&sample) != 0U);
    assert(memcmp(&sample, &before, sizeof(sample)) == 0);
    gIicError = 0U;
}

static void testMagFailureKeepsAccelGyroSample(void)
{
    IMU_DATA_t sample = {0};
    bsp_imu_diag_t diag = {0};

    resetModel();
    bsp_imu_select_device(IMU_DEVICE_ICM20948);
    assert(UserICM20948.Init() == 0U);

    gMagIicError = 1U;
    assert(bsp_imu_update_9axis_checked(&sample) == 0U);
    assert(fabsf(sample.accel.x - 9.8f) < 0.1f);
    assert(fabsf(sample.gyro.x - (1000.0f * 3.14159265f / 180.0f)) <
           0.01f);
    bsp_imu_get_diag(&diag);
    assert(diag.mag_available == 0U);
}

int main(void)
{
    testInitializationAndSample();
    testFailedSampleDoesNotModifyOutput();
    testMagFailureKeepsAccelGyroSample();
    puts("bsp imu tests passed");
    return 0;
}

#endif /* BSP_IMU_HOST_TEST */
