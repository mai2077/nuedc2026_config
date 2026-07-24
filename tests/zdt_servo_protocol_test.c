#ifdef ZDT_SERVO_PROTOCOL_HOST_TEST

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ti_msp_dl_config.h"
#include "bsp_ZDT_Servo_emm_v5.h"

UART_Regs gTestUART0;
UART_Regs gTestUART1;

static uint8_t gStep1Transmitted[32];
static size_t gStep1TransmittedLength;
static uint8_t gResv1Transmitted[32];
static size_t gResv1TransmittedLength;

void DL_UART_Main_transmitDataBlocking(UART_Regs *uart, uint8_t data)
{
    if (uart == STEP1_UART_INST) {
        assert(gStep1TransmittedLength < sizeof(gStep1Transmitted));
        gStep1Transmitted[gStep1TransmittedLength++] = data;
        return;
    }
    assert(uart == RESV1_UART_INST);
    assert(gResv1TransmittedLength < sizeof(gResv1Transmitted));
    gResv1Transmitted[gResv1TransmittedLength++] = data;
}

static void assertFrame(size_t actualLength, const uint8_t *actual,
    const uint8_t *expected, size_t expectedLength)
{
    assert(actualLength == expectedLength);
    assert(memcmp(actual, expected, expectedLength) == 0);
}

static void testQuickPositionFrames(void)
{
    uint8_t frame[16];
    static const uint8_t parameters[] = {
        0x01U, 0xF1U, 0x00U, 0x3CU, 0x0AU, 0x02U, 0x00U, 0x6BU};
    static const uint8_t positive[] = {
        0x01U, 0xFCU, 0x00U, 0x00U, 0x0CU, 0x80U, 0x6BU};
    static const uint8_t negative[] = {
        0x01U, 0xFCU, 0xFFU, 0xFFU, 0xF3U, 0x80U, 0x6BU};

    assertFrame(BSP_EmmV5_Build_SetQPosParams(0x01U, 60U, 10U,
        BSP_EMM_V5_QPOS_REL_CURRENT, false, frame, sizeof(frame)),
        frame, parameters, sizeof(parameters));
    assertFrame(BSP_EmmV5_Build_QPosControl(
        0x01U, 3200, frame, sizeof(frame)),
        frame, positive, sizeof(positive));
    assertFrame(BSP_EmmV5_Build_QPosControl(
        0x01U, -3200, frame, sizeof(frame)),
        frame, negative, sizeof(negative));
}

static void testVelocityStopAndReadFrames(void)
{
    uint8_t frame[16];
    static const uint8_t forward[] = {
        0x01U, 0xF6U, 0x00U, 0x00U, 0x3CU, 0x0AU, 0x00U, 0x6BU};
    static const uint8_t reverse[] = {
        0x01U, 0xF6U, 0x01U, 0x00U, 0x3CU, 0x0AU, 0x00U, 0x6BU};
    static const uint8_t stop[] = {
        0x01U, 0xFEU, 0x98U, 0x00U, 0x6BU};
    static const uint8_t readPosition[] = {0x01U, 0x36U, 0x6BU};

    assertFrame(BSP_EmmV5_Build_VelControl(
        0x01U, 0U, 60U, 10U, false, frame, sizeof(frame)),
        frame, forward, sizeof(forward));
    assertFrame(BSP_EmmV5_Build_VelControl(
        0x01U, 1U, 60U, 10U, false, frame, sizeof(frame)),
        frame, reverse, sizeof(reverse));
    assertFrame(BSP_EmmV5_Build_StopNow(
        0x01U, false, frame, sizeof(frame)),
        frame, stop, sizeof(stop));
    assertFrame(BSP_EmmV5_Build_ReadSysParam(
        0x01U, BSP_EMM_V5_SYS_CPOS, frame, sizeof(frame)),
        frame, readPosition, sizeof(readPosition));
}

static void testPositionAndSynchronousFrames(void)
{
    uint8_t frame[16];
    static const uint8_t position[] = {
        0x01U, 0xFDU, 0x01U, 0x00U, 0x3CU, 0x0AU,
        0x00U, 0x00U, 0x0CU, 0x80U, 0x00U, 0x00U, 0x6BU};
    static const uint8_t synchronous[] = {0x01U, 0xFFU, 0x66U, 0x6BU};

    assertFrame(BSP_EmmV5_Build_PosControl(0x01U, 1U, 60U, 10U,
        3200U, BSP_EMM_V5_POSITION_RELATIVE, false,
        frame, sizeof(frame)), frame, position, sizeof(position));
    assertFrame(BSP_EmmV5_Build_SynchronousMotion(
        0x01U, frame, sizeof(frame)),
        frame, synchronous, sizeof(synchronous));
}

static void testReferenceModeConstants(void)
{
    assert(BSP_EMM_V5_QPOS_REL_LAST == 0U);
    assert(BSP_EMM_V5_QPOS_ABSOLUTE == 1U);
    assert(BSP_EMM_V5_QPOS_REL_CURRENT == 2U);
    assert(BSP_EMM_V5_DIR_CW == 0U);
    assert(BSP_EMM_V5_DIR_CCW == 1U);
    assert(BSP_EMM_V5_POS_RELATIVE == 0U);
    assert(BSP_EMM_V5_POS_ABSOLUTE == 1U);
}

static void testInvalidBuilderArguments(void)
{
    uint8_t frame[4];

    assert(BSP_EmmV5_Build_StopNow(
        0x01U, false, NULL, sizeof(frame)) == 0U);
    assert(BSP_EmmV5_Build_StopNow(
        0x01U, false, frame, sizeof(frame)) == 0U);
    assert(BSP_EmmV5_Build_ReadSysParam(0x01U,
        (BSPEmmV5SysParam)0xFFU, frame, sizeof(frame)) == 0U);
}

static void testSendAndLastFrameSnapshot(void)
{
    uint8_t snapshot[16];
    static const uint8_t stop[] = {
        0x01U, 0xFEU, 0x98U, 0x00U, 0x6BU};

    gStep1TransmittedLength = 0U;
    gResv1TransmittedLength = 0U;
    assert(BSP_EmmV5_StopNow(0x01U, false) == BSP_ZDT_SERVO_OK);
    assertFrame(gStep1TransmittedLength, gStep1Transmitted,
        stop, sizeof(stop));
    assertFrame(gResv1TransmittedLength, gResv1Transmitted,
        stop, sizeof(stop));
    assertFrame(BSP_ZDT_Servo_getLastTxFrame(snapshot, sizeof(snapshot)),
        snapshot, stop, sizeof(stop));
    assert(BSP_ZDT_Servo_getLastTxFrame(snapshot, 4U) == 0U);
}

int main(void)
{
    testQuickPositionFrames();
    testVelocityStopAndReadFrames();
    testPositionAndSynchronousFrames();
    testReferenceModeConstants();
    testInvalidBuilderArguments();
    testSendAndLastFrameSnapshot();
    puts("zdt servo protocol tests passed");
    return 0;
}

#endif /* ZDT_SERVO_PROTOCOL_HOST_TEST */
