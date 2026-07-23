#ifdef PI_TUNER_HOST_TEST

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app/display.h"
#include "app/pi_tuner.h"
#include "bsp/bsp_imu.h"
#include "bsp/key.h"
#include "control/angle_control.h"
#include "control/line_follow.h"
#include "control/wheel_speed.h"

#define TEST_BT_PACKET_LENGTH          (48U)
#define TEST_BT_PACKET_HEAD            (0xA5U)
#define TEST_BT_PACKET_TAIL            (0x5AU)
#define TEST_BT_RUN_OFFSET             (1U)
#define TEST_BT_SPEED_OFFSET           (2U)
#define TEST_BT_INNER_PID_OFFSET       (6U)
#define TEST_BT_ANGLE_PID_OFFSET       (18U)
#define TEST_BT_LINE_PID_OFFSET        (30U)
#define TEST_BT_TARGET_YAW_OFFSET      (42U)
#define TEST_BT_CHECKSUM_OFFSET        (46U)

static int32_t gRightCounts10ms;
static int32_t gLeftCounts10ms;
static int16_t gMotor1Command;
static int16_t gMotor2Command;
static uint8_t gTrackRawMask;
static uint8_t gBtRx[128];
static size_t gBtRxLength;
static size_t gBtRxIndex;
static uint8_t gDebugRx[128];
static size_t gDebugRxLength;
static size_t gDebugRxIndex;
static float gTestYawDeg;
static uint8_t gTestImuReadResult;
static uint16_t gTestImuInitCount;
static uint8_t gKeyEventMask;

static uint8_t testImuInit(void)
{
    ++gTestImuInitCount;
    return 0U;
}

static void testImuUpdateAttitude(
    IMU_DATA_t sample, ATTITUDE_DATA_t *attitude)
{
    (void)sample;
    if (attitude != 0) {
        attitude->roll = 0.0f;
        attitude->pitch = 0.0f;
        attitude->yaw = gTestYawDeg / 57.295779513082320876f;
    }
}

static IMUInterface_t gTestImuInterface = {
    testImuInit,
    0,
    0,
    0,
    0,
    testImuUpdateAttitude
};

int32_t ENCODER_getRightSpeed10ms(void)
{
    return gRightCounts10ms;
}

int32_t ENCODER_getLeftSpeed10ms(void)
{
    return gLeftCounts10ms;
}

void TB6612_setMotor1(int16_t signedPwm)
{
    gMotor1Command = signedPwm;
}

void TB6612_setMotor2(int16_t signedPwm)
{
    gMotor2Command = signedPwm;
}

void TB6612_stopAll(void)
{
    gMotor1Command = 0;
    gMotor2Command = 0;
}

uint8_t TRACK_readRawMask(void)
{
    return gTrackRawMask;
}

void KEY_init(void)
{
}

bool KEY_scan10ms(uint8_t *stableMask)
{
    if ((stableMask == 0) || (gKeyEventMask == 0U)) {
        return false;
    }
    *stableMask = gKeyEventMask;
    gKeyEventMask = 0U;
    return true;
}

void DEBUG_UART_writeByte(uint8_t data)
{
    (void)data;
}

void DEBUG_UART_writeString(const char *text)
{
    (void)text;
}

void DEBUG_UART_writeUInt32(uint32_t value)
{
    (void)value;
}

void DEBUG_UART_writeInt32(int32_t value)
{
    (void)value;
}

void DEBUG_UART_writeSignedFixed3(int32_t milliValue)
{
    (void)milliValue;
}

uint8_t DEBUG_UART_tryReadByte(uint8_t *data)
{
    if ((data == 0) || (gDebugRxIndex >= gDebugRxLength)) {
        return 0U;
    }
    *data = gDebugRx[gDebugRxIndex++];
    return 1U;
}

void BT_UART_writeByte(uint8_t data)
{
    (void)data;
}

void BT_UART_writeString(const char *text)
{
    (void)text;
}

void BT_UART_writeUInt32(uint32_t value)
{
    (void)value;
}

void BT_UART_writeInt32(int32_t value)
{
    (void)value;
}

void BT_UART_writeSignedFixed3(int32_t milliValue)
{
    (void)milliValue;
}

uint8_t BT_UART_tryReadByte(uint8_t *data)
{
    if ((data == 0) || (gBtRxIndex >= gBtRxLength)) {
        return 0U;
    }
    *data = gBtRx[gBtRxIndex++];
    return 1U;
}

void DISPLAY_reportReceivedLine(DISPLAY_Source source, const char *line)
{
    (void)source;
    (void)line;
}

void DISPLAY_reportPidDecoded(
    DISPLAY_Source source, uint16_t kpX100, uint16_t kiX100,
    uint16_t kdX100)
{
    (void)source;
    (void)kpX100;
    (void)kiX100;
    (void)kdX100;
}

void DISPLAY_reportPidApplied(
    DISPLAY_Source source, uint16_t kpX100, uint16_t kiX100)
{
    (void)source;
    (void)kpX100;
    (void)kiX100;
}

void DISPLAY_reportCommandRejected(
    DISPLAY_Source source, const char *reason)
{
    (void)source;
    (void)reason;
}

pIMUInterface_t bsp_imu_get_interface(void)
{
    return &gTestImuInterface;
}

uint8_t bsp_imu_update_9axis_checked(IMU_DATA_t *sample)
{
    if (sample != 0) {
        memset(sample, 0, sizeof(*sample));
    }
    return gTestImuReadResult;
}

void ANO_sendEulerFrame(
    const ATTITUDE_DATA_t *attitude, uint8_t fusionStatus)
{
    (void)attitude;
    (void)fusionStatus;
}

static void writeU32Le(uint8_t *buffer, uint8_t offset, uint32_t value)
{
    buffer[offset] = (uint8_t)value;
    buffer[offset + 1U] = (uint8_t)(value >> 8);
    buffer[offset + 2U] = (uint8_t)(value >> 16);
    buffer[offset + 3U] = (uint8_t)(value >> 24);
}

static void writeFloatLe(uint8_t *buffer, uint8_t offset, float value)
{
    union {
        float value;
        uint32_t bits;
    } converter;

    converter.value = value;
    writeU32Le(buffer, offset, converter.bits);
}

static void writePidLe(
    uint8_t *buffer, uint8_t offset, float kp, float ki, float kd)
{
    writeFloatLe(buffer, offset, kp);
    writeFloatLe(buffer, (uint8_t)(offset + 4U), ki);
    writeFloatLe(buffer, (uint8_t)(offset + 8U), kd);
}

static void feedBtPacket(uint8_t run, int32_t speedMmps,
    float innerP, float innerI, float innerD,
    float angleP, float angleI, float angleD,
    float lineP, float lineI, float lineD, float targetYawDeg)
{
    uint8_t checksum = 0U;
    uint8_t i;

    memset(gBtRx, 0, sizeof(gBtRx));
    gBtRx[0] = TEST_BT_PACKET_HEAD;
    gBtRx[TEST_BT_RUN_OFFSET] = run;
    writeU32Le(gBtRx, TEST_BT_SPEED_OFFSET, (uint32_t)speedMmps);
    writePidLe(gBtRx, TEST_BT_INNER_PID_OFFSET, innerP, innerI, innerD);
    writePidLe(gBtRx, TEST_BT_ANGLE_PID_OFFSET, angleP, angleI, angleD);
    writePidLe(gBtRx, TEST_BT_LINE_PID_OFFSET, lineP, lineI, lineD);
    writeFloatLe(gBtRx, TEST_BT_TARGET_YAW_OFFSET, targetYawDeg);
    for (i = TEST_BT_RUN_OFFSET; i < TEST_BT_CHECKSUM_OFFSET; ++i) {
        checksum = (uint8_t)(checksum + gBtRx[i]);
    }
    gBtRx[TEST_BT_CHECKSUM_OFFSET] = checksum;
    gBtRx[TEST_BT_PACKET_LENGTH - 1U] = TEST_BT_PACKET_TAIL;
    gBtRxLength = TEST_BT_PACKET_LENGTH;
    gBtRxIndex = 0U;
    PI_TUNER_pollUart();
}

static void resetModel(void)
{
    gRightCounts10ms = 0;
    gLeftCounts10ms = 0;
    gMotor1Command = 0;
    gMotor2Command = 0;
    gTrackRawMask = 0x04U;
    gBtRxLength = 0U;
    gBtRxIndex = 0U;
    gDebugRxLength = 0U;
    gDebugRxIndex = 0U;
    gTestYawDeg = 0.0f;
    gTestImuReadResult = 0U;
    gTestImuInitCount = 0U;
    gKeyEventMask = 0U;
    PI_TUNER_init();
}

static void pressKey(uint8_t keyMask, uint32_t tick10ms)
{
    gKeyEventMask = keyMask;
    PI_TUNER_update10ms(tick10ms);
}

static void startCar(void)
{
    pressKey(KEY_MASK_KEY1, 1U);
    assert(PI_TUNER_isRunning() != 0U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 0);
    assert(WHEEL_SPEED_getLeftTargetMmps() == 0);
}

static void testWaitBiasKeysAndKeyStart(void)
{
    uint16_t step;
    uint32_t revision;

    resetModel();
    assert(PI_TUNER_isRunning() == 0U);
    assert(PI_TUNER_getCurveBiasMmps() == 0);

    pressKey(KEY_MASK_KEY3, 1U);
    assert(PI_TUNER_getCurveBiasMmps() == 5);
    assert(PI_TUNER_getCurveBiasRevision() == 1U);

    pressKey(KEY_MASK_KEY3, 2U);
    assert(PI_TUNER_getCurveBiasMmps() == 10);
    assert(PI_TUNER_getCurveBiasRevision() == 2U);

    pressKey(KEY_MASK_KEY4, 3U);
    assert(PI_TUNER_getCurveBiasMmps() == 5);
    assert(PI_TUNER_getCurveBiasRevision() == 3U);

    for (step = 0U; step < 99U; ++step) {
        pressKey(KEY_MASK_KEY3, (uint32_t)(4U + step));
    }
    assert(PI_TUNER_getCurveBiasMmps() == 500);
    revision = PI_TUNER_getCurveBiasRevision();
    pressKey(KEY_MASK_KEY3, 103U);
    assert(PI_TUNER_getCurveBiasMmps() == 500);
    assert(PI_TUNER_getCurveBiasRevision() == revision);

    pressKey(KEY_MASK_KEY1, 104U);
    assert(PI_TUNER_isRunning() != 0U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 0);
    assert(WHEEL_SPEED_getLeftTargetMmps() == 0);

    pressKey(KEY_MASK_KEY4, 105U);
    assert(PI_TUNER_getCurveBiasMmps() == 500);
}

static void testBluetoothPacketsAreIgnored(void)
{
    resetModel();
    feedBtPacket(1U, 80,
        20.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f,
        2.0f, 0.0f, 0.0f, 0.0f);
    assert(PI_TUNER_getCurveBiasMmps() == 0);
    assert(PI_TUNER_getCurveBiasRevision() == 0U);
}

static void testSensorDirectionLostLineAndRecovery(void)
{
    uint8_t tick;

    resetModel();
    startCar();
    assert(LINE_FOLLOW_setPid(2.0f, 0.0f, 0.0f) != 0U);

    gTrackRawMask = 0x04U;
    PI_TUNER_update10ms(2U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 500);
    assert(WHEEL_SPEED_getLeftTargetMmps() == 500);

    gTrackRawMask = 0x01U;
    PI_TUNER_update10ms(3U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 414);
    assert(WHEEL_SPEED_getLeftTargetMmps() == 586);

    gTrackRawMask = 0x10U;
    PI_TUNER_update10ms(4U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 586);
    assert(WHEEL_SPEED_getLeftTargetMmps() == 414);

    gTrackRawMask = 0x00U;
    for (tick = 1U; tick < LINE_FOLLOW_LOST_STOP_TICKS; ++tick) {
        PI_TUNER_update10ms((uint32_t)(4U + tick));
        assert(WHEEL_SPEED_getRightTargetMmps() == 586);
        assert(WHEEL_SPEED_getLeftTargetMmps() == 414);
    }
    PI_TUNER_update10ms(
        (uint32_t)(4U + LINE_FOLLOW_LOST_STOP_TICKS));
    assert(WHEEL_SPEED_getRightTargetMmps() == 0);
    assert(WHEEL_SPEED_getLeftTargetMmps() == 0);
    assert(gMotor1Command == 0);
    assert(gMotor2Command == 0);

    gTrackRawMask = 0x04U;
    PI_TUNER_update10ms(
        (uint32_t)(5U + LINE_FOLLOW_LOST_STOP_TICKS));
    assert(WHEEL_SPEED_getRightTargetMmps() == 500);
    assert(WHEEL_SPEED_getLeftTargetMmps() == 500);
}

static void testCornerPatternStaysInLineFollowing(void)
{
    uint8_t tick;

    resetModel();
    startCar();
    assert(LINE_FOLLOW_setPid(2.0f, 0.0f, 0.0f) != 0U);
    gTrackRawMask = 0x1CU;

    for (tick = 2U; tick < 22U; ++tick) {
        PI_TUNER_update10ms(tick);
        assert(WHEEL_SPEED_getRightTargetMmps() == 539);
        assert(WHEEL_SPEED_getLeftTargetMmps() == 461);
    }
}

static void testLineFollowingDoesNotInitializeImu(void)
{
    resetModel();
    startCar();
    PI_TUNER_update10ms(2U);
    assert(gTestImuInitCount == 0U);
}

int main(void)
{
    testWaitBiasKeysAndKeyStart();
    testBluetoothPacketsAreIgnored();
    testSensorDirectionLostLineAndRecovery();
    testCornerPatternStaysInLineFollowing();
    testLineFollowingDoesNotInitializeImu();
    puts("pi tuner line-follow tests passed");
    return 0;
}

#endif /* PI_TUNER_HOST_TEST */
