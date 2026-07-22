#include "bsp_iic.h"

#include <limits.h>
#include <stddef.h>

#include "ti_msp_dl_config.h"

#define BSP_IIC_ERRATA_DELAY_CYCLES (3U)

#define BSP_IIC_RECOVERY_INTERRUPTS \
    (DL_I2C_INTERRUPT_CONTROLLER_NACK | \
     DL_I2C_INTERRUPT_CONTROLLER_STOP | \
     DL_I2C_INTERRUPT_CONTROLLER_ARBITRATION_LOST | \
     DL_I2C_INTERRUPT_TIMEOUT_A | DL_I2C_INTERRUPT_TIMEOUT_B)

static uint32_t iic_timeout_count(unsigned long timeout_ms)
{
    uint32_t counts_per_ms = CPUCLK_FREQ / 1000U;

    if (counts_per_ms == 0U) {
        counts_per_ms = 1U;
    }

    if (timeout_ms == 0UL) {
        timeout_ms = 1UL;
    }
    if (timeout_ms > (unsigned long)(UINT32_MAX / counts_per_ms)) {
        return UINT32_MAX;
    }
    return (uint32_t)timeout_ms * counts_per_ms;
}

static void iic_recover(void)
{
    /* Force a STOP before clearing a no-STOP address phase or a wedged RX. */
    DL_I2C_enableStopCondition(ICM_I2C_INST);
    delay_cycles(BSP_IIC_ERRATA_DELAY_CYCLES);
    DL_I2C_resetControllerTransfer(ICM_I2C_INST);
    DL_I2C_flushControllerTXFIFO(ICM_I2C_INST);
    DL_I2C_flushControllerRXFIFO(ICM_I2C_INST);
    DL_I2C_clearInterruptStatus(ICM_I2C_INST, BSP_IIC_RECOVERY_INTERRUPTS);
}

static IIC_Status_t iic_wait_idle(uint32_t *budget)
{
    while (*budget > 0U) {
        uint32_t status = DL_I2C_getControllerStatus(ICM_I2C_INST);

        if ((status & DL_I2C_CONTROLLER_STATUS_IDLE) != 0U) {
            return IIC_OK;
        }
        --(*budget);
    }

    return IIC_TIMEOUT;
}

static IIC_Status_t iic_wait_complete(uint32_t *budget)
{
    while (*budget > 0U) {
        uint32_t status = DL_I2C_getControllerStatus(ICM_I2C_INST);

        if ((status & DL_I2C_CONTROLLER_STATUS_ERROR) != 0U) {
            return IIC_ERR;
        }
        if ((status & DL_I2C_CONTROLLER_STATUS_BUSY) == 0U) {
            return IIC_OK;
        }
        --(*budget);
    }

    return IIC_TIMEOUT;
}

static uint32_t iic_address_7bit(uint16_t address_8bit)
{
    return ((uint32_t)address_8bit >> 1U) & 0x7FU;
}

static IIC_Status_t iic_start_tx_budget(uint16_t DevAddress,
                                        uint8_t *pData,
                                        uint16_t Size,
                                        DL_I2C_CONTROLLER_STOP stop,
                                        uint32_t *budget)
{
    IIC_Status_t status;

    if ((pData == NULL) || (Size == 0U)) {
        return IIC_ERR;
    }

    status = iic_wait_idle(budget);
    if (status != IIC_OK) {
        iic_recover();
        return status;
    }

    DL_I2C_flushControllerTXFIFO(ICM_I2C_INST);
    if (DL_I2C_fillControllerTXFIFO(ICM_I2C_INST, pData, Size) != Size) {
        iic_recover();
        return IIC_ERR;
    }
    DL_I2C_startControllerTransferAdvanced(ICM_I2C_INST,
        iic_address_7bit(DevAddress), DL_I2C_CONTROLLER_DIRECTION_TX,
        Size, DL_I2C_CONTROLLER_START_ENABLE, stop,
        DL_I2C_CONTROLLER_ACK_DISABLE);
    delay_cycles(BSP_IIC_ERRATA_DELAY_CYCLES);

    status = iic_wait_complete(budget);
    if (status != IIC_OK) {
        iic_recover();
    }
    return status;
}

static IIC_Status_t iic_start_tx(uint16_t DevAddress,
                                 uint8_t *pData,
                                 uint16_t Size,
                                 DL_I2C_CONTROLLER_STOP stop,
                                 unsigned long Timeout)
{
    uint32_t budget = iic_timeout_count(Timeout);

    return iic_start_tx_budget(DevAddress, pData, Size, stop, &budget);
}

static IIC_Status_t iic_start_rx_budget(uint16_t DevAddress,
                                        uint8_t *pData,
                                        uint16_t Size,
                                        uint32_t *budget,
                                        uint8_t wait_for_idle)
{
    uint16_t index;
    IIC_Status_t status;

    if ((pData == NULL) || (Size == 0U)) {
        return IIC_ERR;
    }

    if (wait_for_idle != 0U) {
        status = iic_wait_idle(budget);
        if (status != IIC_OK) {
            iic_recover();
            return status;
        }
    }

    DL_I2C_flushControllerRXFIFO(ICM_I2C_INST);
    DL_I2C_startControllerTransferAdvanced(ICM_I2C_INST,
        iic_address_7bit(DevAddress), DL_I2C_CONTROLLER_DIRECTION_RX,
        Size, DL_I2C_CONTROLLER_START_ENABLE,
        DL_I2C_CONTROLLER_STOP_ENABLE, DL_I2C_CONTROLLER_ACK_DISABLE);
    delay_cycles(BSP_IIC_ERRATA_DELAY_CYCLES);

    for (index = 0U; index < Size; ++index) {
        while (DL_I2C_isControllerRXFIFOEmpty(ICM_I2C_INST)) {
            uint32_t controller_status =
                DL_I2C_getControllerStatus(ICM_I2C_INST);

            if ((controller_status & DL_I2C_CONTROLLER_STATUS_ERROR) !=
                0U) {
                iic_recover();
                return IIC_ERR;
            }
            if (*budget == 0U) {
                iic_recover();
                return IIC_TIMEOUT;
            }
            --(*budget);
        }
        pData[index] = DL_I2C_receiveControllerData(ICM_I2C_INST);
    }

    status = iic_wait_complete(budget);
    if (status != IIC_OK) {
        iic_recover();
    }
    return status;
}

static IIC_Status_t iic_start_rx(uint16_t DevAddress,
                                 uint8_t *pData,
                                 uint16_t Size,
                                 unsigned long Timeout,
                                 uint8_t wait_for_idle)
{
    uint32_t budget = iic_timeout_count(Timeout);

    return iic_start_rx_budget(
        DevAddress, pData, Size, &budget, wait_for_idle);
}

static IIC_Status_t iic_write(uint16_t DevAddress,
                              uint8_t *pData,
                              uint16_t Size,
                              unsigned long Timeout)
{
    return iic_start_tx(DevAddress, pData, Size,
                        DL_I2C_CONTROLLER_STOP_ENABLE, Timeout);
}

static IIC_Status_t iic_read(uint16_t DevAddress,
                             uint8_t *pData,
                             uint16_t Size,
                             unsigned long Timeout)
{
    return iic_start_rx(DevAddress, pData, Size, Timeout, 1U);
}

static IIC_Status_t iic_write_reg(uint16_t DevAddress,
                                  uint16_t MemAddress,
                                  uint8_t *pData,
                                  uint16_t Size,
                                  unsigned long Timeout)
{
    uint8_t packet[BSP_IIC_MAX_REGISTER_WRITE + 1U];
    uint16_t index;

    if ((MemAddress > 0xFFU) || (pData == NULL) || (Size == 0U) ||
        (Size > BSP_IIC_MAX_REGISTER_WRITE)) {
        return IIC_ERR;
    }

    packet[0] = (uint8_t)MemAddress;
    for (index = 0U; index < Size; ++index) {
        packet[index + 1U] = pData[index];
    }

    return iic_start_tx(DevAddress, packet, (uint16_t)(Size + 1U),
                        DL_I2C_CONTROLLER_STOP_ENABLE, Timeout);
}

static IIC_Status_t iic_read_reg(uint16_t DevAddress,
                                 uint16_t MemAddress,
                                 uint8_t *pData,
                                 uint16_t Size,
                                 unsigned long Timeout)
{
    uint8_t register_address;
    IIC_Status_t status;
    uint32_t budget = iic_timeout_count(Timeout);

    if ((MemAddress > 0xFFU) || (pData == NULL) || (Size == 0U)) {
        return IIC_ERR;
    }

    register_address = (uint8_t)MemAddress;
    status = iic_start_tx_budget(DevAddress, &register_address, 1U,
                                 DL_I2C_CONTROLLER_STOP_DISABLE, &budget);
    if (status != IIC_OK) {
        return status;
    }

    return iic_start_rx_budget(DevAddress, pData, Size, &budget, 0U);
}

static void iic_delay_ms(uint16_t ms)
{
    while (ms > 0U) {
        delay_cycles(CPUCLK_FREQ / 1000U);
        --ms;
    }
}

IICInterface_t UserII2Dev = {
    .write = iic_write,
    .read = iic_read,
    .write_reg = iic_write_reg,
    .read_reg = iic_read_reg,
    .delay_ms = iic_delay_ms
};
