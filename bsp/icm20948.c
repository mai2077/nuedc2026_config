#include "icm20948.h"

#include <stddef.h>
#include <stdint.h>

#include "bsp_iic.h"
#include "bsp_imu.h"
#include "icm_20948_reg.h"

#define ICM20948_ADDRESS_8BIT (ICM20948_DEV << 1U)
#define ICM20948_WHO_AM_I_VALUE (0xEAU)
#define ICM20948_PWR_MGMT_1_VALUE (0x01U)
#define ICM20948_PWR_MGMT_2_VALUE (0x00U)
#define ICM20948_GYRO_DIV_VALUE (0x04U)
#define ICM20948_GYRO_CONFIG_VALUE (0x1FU)

static ICM20948_Status mapIicStatus(IIC_Status_t status)
{
    switch (status) {
        case IIC_OK:
            return ICM20948_STATUS_OK;
        case IIC_TIMEOUT:
            return ICM20948_STATUS_TIMEOUT;
        case IIC_BUSY:
        case IIC_ERR:
        default:
            return ICM20948_STATUS_I2C_ERROR;
    }
}

static ICM20948_Status writeRegister(uint16_t address,
                                     uint8_t reg,
                                     uint8_t value,
                                     unsigned long timeout)
{
    return mapIicStatus(UserII2Dev.write_reg(address, reg, &value, 1U,
                                             timeout));
}

static ICM20948_Status readRegisters(uint16_t address,
                                     uint8_t reg,
                                     uint8_t *data,
                                     uint16_t length,
                                     unsigned long timeout)
{
    return mapIicStatus(UserII2Dev.read_reg(address, reg, data, length,
                                            timeout));
}

ICM20948_Status ICM20948_init(uint8_t *whoAmI)
{
    bsp_imu_diag_t diag;

    if (whoAmI == NULL) {
        return ICM20948_STATUS_INVALID_ARGUMENT;
    }

    bsp_imu_select_device(IMU_DEVICE_ICM20948);
    if (UserICM20948.Init() != 0U) {
        bsp_imu_get_diag(&diag);
        *whoAmI = diag.last_who_am_i;
        if (diag.last_iic_status != IIC_OK) {
            return mapIicStatus(diag.last_iic_status);
        }
        if (*whoAmI != ICM20948_WHO_AM_I_VALUE) {
            return ICM20948_STATUS_WHO_AM_I_MISMATCH;
        }
        return ICM20948_STATUS_I2C_ERROR;
    }

    bsp_imu_get_diag(&diag);
    *whoAmI = diag.last_who_am_i;
    return (*whoAmI == ICM20948_WHO_AM_I_VALUE) ?
        ICM20948_STATUS_OK : ICM20948_STATUS_WHO_AM_I_MISMATCH;
}

ICM20948_Status ICM20948_selfCheck(uint8_t *whoAmI)
{
    uint8_t powerManagement1 = 0U;
    uint8_t powerManagement2 = 0U;
    uint8_t gyroDivider = 0U;
    uint8_t gyroConfig = 0U;
    ICM20948_Status status;

    if (whoAmI == NULL) {
        return ICM20948_STATUS_INVALID_ARGUMENT;
    }

    status = writeRegister(ICM20948_ADDRESS_8BIT,
                           REG_BANK_SEL, REG_VAL_SELECT_BANK_0, 100U);
    if (status != ICM20948_STATUS_OK) {
        return status;
    }
    status = readRegisters(ICM20948_ADDRESS_8BIT, WHO_AM_I,
                           whoAmI, 1U, 100U);
    if (status != ICM20948_STATUS_OK) {
        return status;
    }
    if (*whoAmI != ICM20948_WHO_AM_I_VALUE) {
        return ICM20948_STATUS_WHO_AM_I_MISMATCH;
    }
    status = readRegisters(ICM20948_ADDRESS_8BIT, PWR_MGMT_1,
                           &powerManagement1, 1U, 100U);
    if (status != ICM20948_STATUS_OK) {
        return status;
    }
    status = readRegisters(ICM20948_ADDRESS_8BIT, PWR_MGMT_2,
                           &powerManagement2, 1U, 100U);
    if (status != ICM20948_STATUS_OK) {
        return status;
    }
    status = writeRegister(ICM20948_ADDRESS_8BIT,
                           REG_BANK_SEL, REG_VAL_SELECT_BANK_2, 100U);
    if (status != ICM20948_STATUS_OK) {
        return status;
    }
    status = readRegisters(ICM20948_ADDRESS_8BIT, GYRO_SMPLRT_DIV,
                           &gyroDivider, 1U, 100U);
    if (status == ICM20948_STATUS_OK) {
        status = readRegisters(ICM20948_ADDRESS_8BIT, GYRO_CONFIG_1,
                               &gyroConfig, 1U, 100U);
    }

    {
        ICM20948_Status restore = writeRegister(
            ICM20948_ADDRESS_8BIT, REG_BANK_SEL,
            REG_VAL_SELECT_BANK_0, 100U);
        if (status != ICM20948_STATUS_OK) {
            return status;
        }
        if (restore != ICM20948_STATUS_OK) {
            return restore;
        }
    }

    if ((powerManagement1 != ICM20948_PWR_MGMT_1_VALUE) ||
        (powerManagement2 != ICM20948_PWR_MGMT_2_VALUE) ||
        (gyroDivider != ICM20948_GYRO_DIV_VALUE) ||
        (gyroConfig != ICM20948_GYRO_CONFIG_VALUE)) {
        return ICM20948_STATUS_CONFIG_MISMATCH;
    }

    return ICM20948_STATUS_OK;
}

ICM20948_Status ICM20948_readGyro(ICM20948_GyroSample *sample)
{
    uint8_t data[6];
    ICM20948_Status status;

    if (sample == NULL) {
        return ICM20948_STATUS_INVALID_ARGUMENT;
    }

    status = writeRegister(ICM20948_ADDRESS_8BIT,
                           REG_BANK_SEL, REG_VAL_SELECT_BANK_0, 100U);
    if (status != ICM20948_STATUS_OK) {
        return status;
    }
    status = readRegisters(ICM20948_ADDRESS_8BIT, GYRO_XOUT_H,
                           data, sizeof(data), 100U);
    if (status != ICM20948_STATUS_OK) {
        return status;
    }

    sample->rawX = (int16_t)(((uint16_t)data[0] << 8U) | data[1]);
    sample->rawY = (int16_t)(((uint16_t)data[2] << 8U) | data[3]);
    sample->rawZ = (int16_t)(((uint16_t)data[4] << 8U) | data[5]);
    sample->milliDpsX = ICM20948_rawToMilliDps(sample->rawX);
    sample->milliDpsY = ICM20948_rawToMilliDps(sample->rawY);
    sample->milliDpsZ = ICM20948_rawToMilliDps(sample->rawZ);
    return ICM20948_STATUS_OK;
}

int32_t ICM20948_rawToMilliDps(int16_t rawValue)
{
    int64_t scaled = (int64_t)rawValue * 125000LL;

    return (int32_t)(scaled / 2048LL);
}
