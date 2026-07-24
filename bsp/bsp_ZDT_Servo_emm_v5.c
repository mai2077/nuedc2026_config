#include "bsp_ZDT_Servo_emm_v5.h"

#include <string.h>

#include "ti_msp_dl_config.h"

static uint8_t gLastTxFrame[BSP_ZDT_SERVO_MAX_FRAME_LENGTH];
static size_t gLastTxLength;

static bool hasSpace(size_t need, size_t have)
{
    return have >= need;
}

static BSP_ZDT_Servo_Status transmitFrame(
    const uint8_t *frame, size_t length)
{
    size_t index;

    if ((frame == NULL) || (length == 0U) ||
        (length > sizeof(gLastTxFrame))) {
        return BSP_ZDT_SERVO_INVALID_ARGUMENT;
    }

    memcpy(gLastTxFrame, frame, length);
    gLastTxLength = length;
    for (index = 0U; index < length; ++index) {
        DL_UART_Main_transmitDataBlocking(STEP1_UART_INST, frame[index]);
    }
    for (index = 0U; index < length; ++index) {
        DL_UART_Main_transmitDataBlocking(RESV1_UART_INST, frame[index]);
    }
    return BSP_ZDT_SERVO_OK;
}

size_t BSP_EmmV5_Build_SetQPosParams(uint8_t addr, uint16_t vel,
    uint8_t acc, uint8_t raF, bool snF, uint8_t *buf, size_t bufSize)
{
    if ((buf == NULL) || !hasSpace(8U, bufSize)) {
        return 0U;
    }

    buf[0] = addr;
    buf[1] = 0xF1U;
    buf[2] = (uint8_t)(vel >> 8U);
    buf[3] = (uint8_t)vel;
    buf[4] = acc;
    buf[5] = raF;
    buf[6] = snF ? 1U : 0U;
    buf[7] = BSP_EMM_V5_FRAME_END;
    return 8U;
}

size_t BSP_EmmV5_Build_QPosControl(uint8_t addr, int32_t clk,
    uint8_t *buf, size_t bufSize)
{
    uint32_t raw;

    if ((buf == NULL) || !hasSpace(7U, bufSize)) {
        return 0U;
    }

    raw = (uint32_t)clk;
    buf[0] = addr;
    buf[1] = 0xFCU;
    buf[2] = (uint8_t)(raw >> 24U);
    buf[3] = (uint8_t)(raw >> 16U);
    buf[4] = (uint8_t)(raw >> 8U);
    buf[5] = (uint8_t)raw;
    buf[6] = BSP_EMM_V5_FRAME_END;
    return 7U;
}

size_t BSP_EmmV5_Build_VelControl(uint8_t addr, uint8_t dir,
    uint16_t vel, uint8_t acc, bool snF, uint8_t *buf, size_t bufSize)
{
    if ((buf == NULL) || !hasSpace(8U, bufSize)) {
        return 0U;
    }

    buf[0] = addr;
    buf[1] = 0xF6U;
    buf[2] = (dir != 0U) ? 1U : 0U;
    buf[3] = (uint8_t)(vel >> 8U);
    buf[4] = (uint8_t)vel;
    buf[5] = acc;
    buf[6] = snF ? 1U : 0U;
    buf[7] = BSP_EMM_V5_FRAME_END;
    return 8U;
}

size_t BSP_EmmV5_Build_PosControl(uint8_t addr, uint8_t dir,
    uint16_t vel, uint8_t acc, uint32_t clk, uint8_t raF, bool snF,
    uint8_t *buf, size_t bufSize)
{
    if ((buf == NULL) || !hasSpace(13U, bufSize)) {
        return 0U;
    }

    buf[0] = addr;
    buf[1] = 0xFDU;
    buf[2] = (dir != 0U) ? 1U : 0U;
    buf[3] = (uint8_t)(vel >> 8U);
    buf[4] = (uint8_t)vel;
    buf[5] = acc;
    buf[6] = (uint8_t)(clk >> 24U);
    buf[7] = (uint8_t)(clk >> 16U);
    buf[8] = (uint8_t)(clk >> 8U);
    buf[9] = (uint8_t)clk;
    buf[10] = (raF != 0U) ? 1U : 0U;
    buf[11] = snF ? 1U : 0U;
    buf[12] = BSP_EMM_V5_FRAME_END;
    return 13U;
}

size_t BSP_EmmV5_Build_StopNow(uint8_t addr, bool snF,
    uint8_t *buf, size_t bufSize)
{
    if ((buf == NULL) || !hasSpace(5U, bufSize)) {
        return 0U;
    }

    buf[0] = addr;
    buf[1] = 0xFEU;
    buf[2] = 0x98U;
    buf[3] = snF ? 1U : 0U;
    buf[4] = BSP_EMM_V5_FRAME_END;
    return 5U;
}

size_t BSP_EmmV5_Build_SynchronousMotion(uint8_t addr,
    uint8_t *buf, size_t bufSize)
{
    if ((buf == NULL) || !hasSpace(4U, bufSize)) {
        return 0U;
    }

    buf[0] = addr;
    buf[1] = 0xFFU;
    buf[2] = 0x66U;
    buf[3] = BSP_EMM_V5_FRAME_END;
    return 4U;
}

size_t BSP_EmmV5_Build_ReadSysParam(uint8_t addr,
    BSPEmmV5SysParam param, uint8_t *buf, size_t bufSize)
{
    uint8_t command;

    if ((buf == NULL) || !hasSpace(3U, bufSize)) {
        return 0U;
    }

    switch (param) {
        case BSP_EMM_V5_SYS_VBUS:
            command = 0x24U;
            break;
        case BSP_EMM_V5_SYS_CBUS:
            command = 0x26U;
            break;
        case BSP_EMM_V5_SYS_VEL:
            command = 0x35U;
            break;
        case BSP_EMM_V5_SYS_CPOS:
            command = 0x36U;
            break;
        default:
            return 0U;
    }

    buf[0] = addr;
    buf[1] = command;
    buf[2] = BSP_EMM_V5_FRAME_END;
    return 3U;
}

BSP_ZDT_Servo_Status BSP_EmmV5_SetQPosParams(uint8_t addr,
    uint16_t vel, uint8_t acc, uint8_t raF, bool snF)
{
    uint8_t frame[8];
    size_t length = BSP_EmmV5_Build_SetQPosParams(
        addr, vel, acc, raF, snF, frame, sizeof(frame));

    return transmitFrame(frame, length);
}

BSP_ZDT_Servo_Status BSP_EmmV5_QPosControl(uint8_t addr, int32_t clk)
{
    uint8_t frame[7];
    size_t length = BSP_EmmV5_Build_QPosControl(
        addr, clk, frame, sizeof(frame));

    return transmitFrame(frame, length);
}

BSP_ZDT_Servo_Status BSP_EmmV5_VelControl(uint8_t addr, uint8_t dir,
    uint16_t vel, uint8_t acc, bool snF)
{
    uint8_t frame[8];
    size_t length = BSP_EmmV5_Build_VelControl(
        addr, dir, vel, acc, snF, frame, sizeof(frame));

    return transmitFrame(frame, length);
}

BSP_ZDT_Servo_Status BSP_EmmV5_PosControl(uint8_t addr, uint8_t dir,
    uint16_t vel, uint8_t acc, uint32_t clk, uint8_t raF, bool snF)
{
    uint8_t frame[13];
    size_t length = BSP_EmmV5_Build_PosControl(addr, dir, vel, acc,
        clk, raF, snF, frame, sizeof(frame));

    return transmitFrame(frame, length);
}

BSP_ZDT_Servo_Status BSP_EmmV5_StopNow(uint8_t addr, bool snF)
{
    uint8_t frame[5];
    size_t length = BSP_EmmV5_Build_StopNow(
        addr, snF, frame, sizeof(frame));

    return transmitFrame(frame, length);
}

BSP_ZDT_Servo_Status BSP_EmmV5_SynchronousMotion(uint8_t addr)
{
    uint8_t frame[4];
    size_t length = BSP_EmmV5_Build_SynchronousMotion(
        addr, frame, sizeof(frame));

    return transmitFrame(frame, length);
}

BSP_ZDT_Servo_Status BSP_EmmV5_ReadSysParam(
    uint8_t addr, BSPEmmV5SysParam param)
{
    uint8_t frame[3];
    size_t length = BSP_EmmV5_Build_ReadSysParam(
        addr, param, frame, sizeof(frame));

    return transmitFrame(frame, length);
}

size_t BSP_ZDT_Servo_getLastTxFrame(uint8_t *buffer, size_t capacity)
{
    if ((buffer == NULL) || (gLastTxLength == 0U) ||
        (capacity < gLastTxLength)) {
        return 0U;
    }

    memcpy(buffer, gLastTxFrame, gLastTxLength);
    return gLastTxLength;
}
