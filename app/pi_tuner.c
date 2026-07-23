#include "pi_tuner.h"

#include "app/display.h"
#include "app/imu_diag_schedule.h"
#include "bsp/ano_protocol.h"
#include "bsp/bsp_imu.h"
#include "bsp/bt_uart.h"
#include "bsp/debug_uart.h"
#include "bsp/encoder.h"
#include "bsp/key.h"
#include "bsp/tb6612.h"
#include "bsp/track.h"
#include "control/angle_control.h"
#include "control/line_follow.h"

#define PI_TUNER_LINE_CAPACITY       (64U)
#define PI_TUNER_TELEMETRY_TICKS     (2U)
#define PI_TUNER_DEBUG_PWM_TICKS     (5U)
#define PI_TUNER_DEBUG_PID_TICKS     (50U)
#define PI_TUNER_FIXED_TARGET_MMPS   (500)
#define PI_TUNER_ANO_HEAD            (0xABU)
#define PI_TUNER_ANO_SOURCE_ADDR     (0x01U)
#define PI_TUNER_ANO_TARGET_ADDR     (0xFEU)
#define PI_TUNER_ANO_PWM_ID          (0x20U)
#define PI_TUNER_ANO_PWM_DATA_LEN    (16U)
#define PI_TUNER_BT_PACKET_HEAD      (0xA5U)
#define PI_TUNER_BT_PACKET_TAIL      (0x5AU)
#define PI_TUNER_BT_LEGACY_PAYLOAD_LEN (41U)
#define PI_TUNER_BT_LEGACY_FRAME_LEN (PI_TUNER_BT_LEGACY_PAYLOAD_LEN + 3U)
#define PI_TUNER_BT_PAYLOAD_LEN      (45U)
#define PI_TUNER_BT_FRAME_LEN        (PI_TUNER_BT_PAYLOAD_LEN + 3U)
#define PI_TUNER_BT_RUN_OFFSET       (1U)
#define PI_TUNER_BT_SPEED_OFFSET     (2U)
#define PI_TUNER_BT_INNER_PID_OFFSET (6U)
#define PI_TUNER_BT_ANGLE_PID_OFFSET (18U)
#define PI_TUNER_BT_LINE_PID_OFFSET  (30U)
#define PI_TUNER_BT_TARGET_YAW_OFFSET (42U)
#define PI_TUNER_RAD_TO_DEG          (57.295779513082320876f)

typedef struct {
    char line[PI_TUNER_LINE_CAPACITY];
    uint8_t length;
    uint8_t overflow;
} PI_TUNER_LineState;

typedef struct {
    float p;
    float i;
    float d;
} PI_TUNER_FloatPid;

typedef struct {
    uint8_t frame[PI_TUNER_BT_FRAME_LEN];
    uint8_t length;
} PI_TUNER_BtPacketState;

typedef enum {
    PI_TUNER_PARSE_OK = 0,
    PI_TUNER_PARSE_MALFORMED,
    PI_TUNER_PARSE_OUT_OF_RANGE
} PI_TUNER_ParseResult;

typedef enum {
    PI_TUNER_DEBUG_MODE_NONE = 0,
    PI_TUNER_DEBUG_MODE_PWM = 1,
    PI_TUNER_DEBUG_MODE_IMU = 2,
    PI_TUNER_DEBUG_MODE_PID = 3
} PI_TUNER_DebugMode;

static uint8_t gRunning;
static int16_t gTargetMmps;
static PI_TUNER_DebugMode gDebugMode;
static uint8_t gDebugOutputStarted;
static uint32_t gLastDebugOutputTick;
#if PI_TUNER_ENABLE_BT_TELEMETRY
static uint8_t gTelemetryStarted;
static uint32_t gLastTelemetryTick;
#endif
static PI_TUNER_LineState gBtLineState;
static PI_TUNER_LineState gDebugLineState;
static PI_TUNER_BtPacketState gBtPacketState;
static PI_TUNER_FloatPid gOuterAnglePid;
static PI_TUNER_FloatPid gOuterLinePid;
static float gTargetYawDeg;
static uint8_t gImuInitialized;
static uint8_t gImuInitAttempted;

static int32_t piTunerFloatToMilli(float value);

static uint8_t piTunerTextEqual(const char *left, const char *right)
{
    while ((*left != '\0') && (*right != '\0')) {
        if (*left != *right) {
            return 0U;
        }
        ++left;
        ++right;
    }
    return (uint8_t)((*left == '\0') && (*right == '\0'));
}

static void piTunerPrintDebugMenu(void)
{
    DEBUG_UART_writeString("\r\nDEBUG MENU\r\n");
    DEBUG_UART_writeString("mode1: PWM ANO frame 50ms\r\n");
    DEBUG_UART_writeString("mode2: IMU ANO frame 20ms\r\n");
    DEBUG_UART_writeString("mode3: PID values 500ms\r\n");
    DEBUG_UART_writeString("mode4+: reserved\r\n");
    DEBUG_UART_writeString("send 1/2/3, mode1/mode2/mode3, or menu\r\n");
}

static void piTunerSelectDebugMode(PI_TUNER_DebugMode mode)
{
    gDebugMode = mode;
    gDebugOutputStarted = 0U;
    gLastDebugOutputTick = 0U;
}

static uint32_t piTunerDebugModePeriodTicks(void)
{
    switch (gDebugMode) {
        case PI_TUNER_DEBUG_MODE_PWM:
            return PI_TUNER_DEBUG_PWM_TICKS;
        case PI_TUNER_DEBUG_MODE_IMU:
            return IMU_DIAG_PERIOD_TICKS;
        case PI_TUNER_DEBUG_MODE_PID:
            return PI_TUNER_DEBUG_PID_TICKS;
        default:
            return 0U;
    }
}

static uint8_t piTunerEnsureImuInitialized(uint8_t reportDebug)
{
    pIMUInterface_t imuInterface;

    if (gImuInitialized != 0U) {
        return 1U;
    }
    if (gImuInitAttempted != 0U) {
        return 0U;
    }

    gImuInitAttempted = 1U;
    imuInterface = bsp_imu_get_interface();
    if ((imuInterface == 0) || (imuInterface->Init == 0)) {
        if (reportDebug != 0U) {
            DEBUG_UART_writeString("[DBG IMU] init unavailable\r\n");
        }
        return 0U;
    }

    if (imuInterface->Init() == 0U) {
        gImuInitialized = 1U;
        if (reportDebug != 0U) {
            DEBUG_UART_writeString("[DBG IMU] init ok\r\n");
        }
        return 1U;
    }

    if (reportDebug != 0U) {
        DEBUG_UART_writeString("[DBG IMU] init failed\r\n");
    }
    return 0U;
}

static uint8_t piTunerFloatIsFinite(float value)
{
    return (uint8_t)(
        (value == value) &&
        (value <= 3.402823466e38F) &&
        (value >= -3.402823466e38F));
}

static uint8_t piTunerPidIsFiniteNonnegative(PI_TUNER_FloatPid pid)
{
    return (uint8_t)(
        (piTunerFloatIsFinite(pid.p) != 0U) &&
        (piTunerFloatIsFinite(pid.i) != 0U) &&
        (piTunerFloatIsFinite(pid.d) != 0U) &&
        (pid.p >= 0.0f) && (pid.i >= 0.0f) && (pid.d >= 0.0f));
}

static uint8_t piTunerReadCurrentYawDeg(float *yawDeg)
{
    IMU_DATA_t sample;
    ATTITUDE_DATA_t attitude;
    pIMUInterface_t imuInterface;

    if (yawDeg == 0) {
        return 0U;
    }
    if (piTunerEnsureImuInitialized(0U) == 0U) {
        return 0U;
    }

    imuInterface = bsp_imu_get_interface();
    if ((imuInterface == 0) || (imuInterface->UpdateAttitude == 0)) {
        return 0U;
    }

    if (bsp_imu_update_9axis_checked(&sample) != 0U) {
        return 0U;
    }

    attitude.roll = 0.0f;
    attitude.pitch = 0.0f;
    attitude.yaw = 0.0f;
    imuInterface->UpdateAttitude(sample, &attitude);
    *yawDeg = attitude.yaw * PI_TUNER_RAD_TO_DEG;
    return 1U;
}

static void piTunerApplyMotorOutputs(void)
{
    TB6612_setMotor1((int16_t)-WHEEL_SPEED_getRightPwm());
    TB6612_setMotor2(WHEEL_SPEED_getLeftPwm());
}

static void piTunerWriteFixed2Integer(int32_t value)
{
    BT_UART_writeInt32(value);
    BT_UART_writeString(".00");
}

static void piTunerWriteTelemetry(void)
{
    WHEEL_SPEED_Diagnostics rightDiagnostics;
    WHEEL_SPEED_Diagnostics leftDiagnostics;
    ANGLE_CONTROL_Output angleOutput;
    float currentYawDeg;

    WHEEL_SPEED_getDiagnostics(WHEEL_SPEED_WHEEL_RIGHT, &rightDiagnostics);
    WHEEL_SPEED_getDiagnostics(WHEEL_SPEED_WHEEL_LEFT, &leftDiagnostics);
    currentYawDeg = 0.0f;
    if (piTunerReadCurrentYawDeg(&currentYawDeg) == 0U) {
        ANGLE_CONTROL_getOutput(&angleOutput);
        currentYawDeg = angleOutput.currentYawDeg;
    }

    piTunerWriteFixed2Integer(rightDiagnostics.inputMmps);
    BT_UART_writeByte((uint8_t)',');
    piTunerWriteFixed2Integer(leftDiagnostics.inputMmps);
    BT_UART_writeByte((uint8_t)',');
    BT_UART_writeSignedFixed3(piTunerFloatToMilli(currentYawDeg));
    BT_UART_writeString("\r\n");
}

static uint16_t piTunerClampPwmU16(int32_t value)
{
    if (value > 10000) {
        return 10000U;
    }
    if (value < 0) {
        return 0U;
    }
    return (uint16_t)value;
}

static void piTunerAnoAppendByte(
    uint8_t *buffer, uint8_t *index, uint8_t value)
{
    buffer[*index] = value;
    ++(*index);
}

static void piTunerAnoAppendU16Le(
    uint8_t *buffer, uint8_t *index, uint16_t value)
{
    piTunerAnoAppendByte(buffer, index, (uint8_t)(value & 0xFFU));
    piTunerAnoAppendByte(buffer, index, (uint8_t)(value >> 8));
}

static void piTunerAnoWriteFrame(const uint8_t *buffer, uint8_t length)
{
    uint8_t i;

    for (i = 0U; i < length; ++i) {
        DEBUG_UART_writeByte(buffer[i]);
    }
}

static int16_t piTunerClampTargetInt32(int32_t targetMmps)
{
    if (targetMmps > WHEEL_SPEED_MAX_TARGET_MMPS) {
        return WHEEL_SPEED_MAX_TARGET_MMPS;
    }
    if (targetMmps < -WHEEL_SPEED_MAX_TARGET_MMPS) {
        return -WHEEL_SPEED_MAX_TARGET_MMPS;
    }
    return (int16_t)targetMmps;
}

static int32_t piTunerReadI32Le(const uint8_t *buffer, uint8_t offset)
{
    uint32_t raw =
        ((uint32_t)buffer[offset]) |
        ((uint32_t)buffer[offset + 1U] << 8) |
        ((uint32_t)buffer[offset + 2U] << 16) |
        ((uint32_t)buffer[offset + 3U] << 24);

    return (int32_t)raw;
}

static float piTunerReadFloatLe(const uint8_t *buffer, uint8_t offset)
{
    union {
        uint32_t raw;
        float value;
    } converter;

    converter.raw =
        ((uint32_t)buffer[offset]) |
        ((uint32_t)buffer[offset + 1U] << 8) |
        ((uint32_t)buffer[offset + 2U] << 16) |
        ((uint32_t)buffer[offset + 3U] << 24);
    return converter.value;
}

static PI_TUNER_FloatPid piTunerReadFloatPid(
    const uint8_t *buffer, uint8_t offset)
{
    PI_TUNER_FloatPid pid;

    pid.p = piTunerReadFloatLe(buffer, offset);
    pid.i = piTunerReadFloatLe(buffer, (uint8_t)(offset + 4U));
    pid.d = piTunerReadFloatLe(buffer, (uint8_t)(offset + 8U));
    return pid;
}

static uint8_t piTunerFloatToX100(
    float value, uint16_t maxX100, uint16_t *valueX100)
{
    float scaled;

    if ((valueX100 == 0) || (value != value) || (value < 0.0f)) {
        return 0U;
    }

    scaled = value * 100.0f + 0.5f;
    if (scaled > (float)maxX100) {
        return 0U;
    }

    *valueX100 = (uint16_t)scaled;
    return 1U;
}

static uint8_t piTunerBtPacketChecksumFrom(
    const uint8_t *frame, uint8_t startIndex, uint8_t checksumIndex)
{
    uint8_t checksum = 0U;
    uint8_t i;

    for (i = startIndex; i < checksumIndex; ++i) {
        checksum = (uint8_t)(checksum + frame[i]);
    }
    return checksum;
}

static uint8_t piTunerBtPacketIsValid(
    const uint8_t *frame, uint8_t frameLength)
{
    uint8_t checksumIndex;
    uint8_t tailIndex;

    if ((frameLength != PI_TUNER_BT_LEGACY_FRAME_LEN) &&
        (frameLength != PI_TUNER_BT_FRAME_LEN)) {
        return 0U;
    }

    checksumIndex = (uint8_t)(frameLength - 2U);
    tailIndex = (uint8_t)(frameLength - 1U);
    return (uint8_t)(
        (frame[0] == PI_TUNER_BT_PACKET_HEAD) &&
        (frame[tailIndex] == PI_TUNER_BT_PACKET_TAIL) &&
        ((frame[checksumIndex] ==
            piTunerBtPacketChecksumFrom(frame, 1U, checksumIndex)) ||
         (frame[checksumIndex] ==
            piTunerBtPacketChecksumFrom(frame, 0U, checksumIndex))));
}

static int32_t piTunerFloatToMilli(float value)
{
    if (value >= 0.0f) {
        return (int32_t)(value * 1000.0f + 0.5f);
    }
    return (int32_t)(value * 1000.0f - 0.5f);
}

static void piTunerDebugWriteFloat3(float value)
{
    DEBUG_UART_writeSignedFixed3(piTunerFloatToMilli(value));
}

static void piTunerDebugWritePid(
    const char *label, PI_TUNER_FloatPid pid)
{
    DEBUG_UART_writeString(label);
    DEBUG_UART_writeString(" P=");
    piTunerDebugWriteFloat3(pid.p);
    DEBUG_UART_writeString(" I=");
    piTunerDebugWriteFloat3(pid.i);
    DEBUG_UART_writeString(" D=");
    piTunerDebugWriteFloat3(pid.d);
}

static void piTunerReportBtPacketDecoded(
    uint8_t run, int16_t targetMmps, PI_TUNER_FloatPid innerPid,
    PI_TUNER_FloatPid anglePid, PI_TUNER_FloatPid linePid,
    float targetYawDeg)
{
    DEBUG_UART_writeString("[BT BIN] RUN=");
    DEBUG_UART_writeUInt32((run != 0U) ? 1U : 0U);
    DEBUG_UART_writeString(" SPEED=");
    DEBUG_UART_writeInt32(targetMmps);
    piTunerDebugWritePid(" INNER", innerPid);
    piTunerDebugWritePid(" ANGLE", anglePid);
    piTunerDebugWritePid(" LINE", linePid);
    DEBUG_UART_writeString(" TARGET=");
    piTunerDebugWriteFloat3(targetYawDeg);
    DEBUG_UART_writeString("\r\n");
}

static void piTunerWritePidPage(void)
{
    WHEEL_SPEED_Diagnostics rightDiagnostics;
    ANGLE_CONTROL_Output angleOutput;

    WHEEL_SPEED_getDiagnostics(WHEEL_SPEED_WHEEL_RIGHT, &rightDiagnostics);
    ANGLE_CONTROL_getOutput(&angleOutput);

    DEBUG_UART_writeString("[PID] RUN=");
    DEBUG_UART_writeUInt32((gRunning != 0U) ? 1U : 0U);
    DEBUG_UART_writeString(" SPEED=");
    DEBUG_UART_writeInt32((int32_t)gTargetMmps);
    DEBUG_UART_writeString(" INNER P=");
    DEBUG_UART_writeSignedFixed3((int32_t)rightDiagnostics.kpX100 * 10);
    DEBUG_UART_writeString(" I=");
    DEBUG_UART_writeSignedFixed3((int32_t)rightDiagnostics.kiX100 * 10);
    DEBUG_UART_writeString(" D=0.000");
    DEBUG_UART_writeString(" ANGLE P=");
    piTunerDebugWriteFloat3(gOuterAnglePid.p);
    DEBUG_UART_writeString(" I=");
    piTunerDebugWriteFloat3(gOuterAnglePid.i);
    DEBUG_UART_writeString(" D=");
    piTunerDebugWriteFloat3(gOuterAnglePid.d);
    DEBUG_UART_writeString(" LINE P=");
    piTunerDebugWriteFloat3(gOuterLinePid.p);
    DEBUG_UART_writeString(" I=");
    piTunerDebugWriteFloat3(gOuterLinePid.i);
    DEBUG_UART_writeString(" D=");
    piTunerDebugWriteFloat3(gOuterLinePid.d);
    DEBUG_UART_writeString(" YAW=");
    piTunerDebugWriteFloat3(angleOutput.currentYawDeg);
    DEBUG_UART_writeString(" TARGET=");
    piTunerDebugWriteFloat3(angleOutput.targetYawDeg);
    DEBUG_UART_writeString(" ERR=");
    piTunerDebugWriteFloat3(angleOutput.errorDeg);
    DEBUG_UART_writeString(" STATE=");
    DEBUG_UART_writeString(ANGLE_CONTROL_stateName(angleOutput.state));
    DEBUG_UART_writeString("\r\n");
}

static void piTunerWriteDebugPwmFrame(void)
{
    uint8_t frame[PI_TUNER_ANO_PWM_DATA_LEN + 8U];
    uint8_t index = 0U;
    uint8_t sumCheck = 0U;
    uint8_t addCheck = 0U;
    uint8_t i;
    WHEEL_SPEED_Diagnostics diagnostics;

    WHEEL_SPEED_getDiagnostics(WHEEL_SPEED_WHEEL_RIGHT, &diagnostics);

    piTunerAnoAppendByte(frame, &index, PI_TUNER_ANO_HEAD);
    piTunerAnoAppendByte(frame, &index, PI_TUNER_ANO_SOURCE_ADDR);
    piTunerAnoAppendByte(frame, &index, PI_TUNER_ANO_TARGET_ADDR);
    piTunerAnoAppendByte(frame, &index, PI_TUNER_ANO_PWM_ID);
    piTunerAnoAppendU16Le(frame, &index, PI_TUNER_ANO_PWM_DATA_LEN);
    piTunerAnoAppendU16Le(frame, &index,
        piTunerClampPwmU16(diagnostics.outputPwm));
    piTunerAnoAppendU16Le(frame, &index,
        piTunerClampPwmU16(WHEEL_SPEED_getLeftPwm()));
    piTunerAnoAppendU16Le(frame, &index, 0U);
    piTunerAnoAppendU16Le(frame, &index, 0U);
    piTunerAnoAppendU16Le(frame, &index, 0U);
    piTunerAnoAppendU16Le(frame, &index, 0U);
    piTunerAnoAppendU16Le(frame, &index, 0U);
    piTunerAnoAppendU16Le(frame, &index, 0U);

    for (i = 0U; i < index; ++i) {
        sumCheck = (uint8_t)(sumCheck + frame[i]);
        addCheck = (uint8_t)(addCheck + sumCheck);
    }
    piTunerAnoAppendByte(frame, &index, sumCheck);
    piTunerAnoAppendByte(frame, &index, addCheck);
    piTunerAnoWriteFrame(frame, index);
}

static void piTunerWriteDebugImuFrame(void)
{
    IMU_DATA_t sample;
    ATTITUDE_DATA_t attitude;
    pIMUInterface_t imuInterface;

    if (piTunerEnsureImuInitialized(1U) == 0U) {
        return;
    }

    imuInterface = bsp_imu_get_interface();
    if ((imuInterface == 0) || (imuInterface->UpdateAttitude == 0)) {
        return;
    }

    if (bsp_imu_update_9axis_checked(&sample) == 0U) {
        attitude.roll = 0.0f;
        attitude.pitch = 0.0f;
        attitude.yaw = 0.0f;
        imuInterface->UpdateAttitude(sample, &attitude);
        ANO_sendEulerFrame(&attitude, 0U);
    }
}

static void piTunerServiceDebugMode(uint32_t tick10ms)
{
    uint32_t periodTicks = piTunerDebugModePeriodTicks();

    if (periodTicks == 0U) {
        return;
    }

    if ((gDebugOutputStarted != 0U) &&
        ((uint32_t)(tick10ms - gLastDebugOutputTick) < periodTicks)) {
        return;
    }

    gDebugOutputStarted = 1U;
    gLastDebugOutputTick = tick10ms;

    switch (gDebugMode) {
        case PI_TUNER_DEBUG_MODE_PWM:
            piTunerWriteDebugPwmFrame();
            break;
        case PI_TUNER_DEBUG_MODE_IMU:
            piTunerWriteDebugImuFrame();
            break;
        case PI_TUNER_DEBUG_MODE_PID:
            piTunerWritePidPage();
            break;
        default:
            break;
    }
}

static void piTunerResetLineState(PI_TUNER_LineState *state)
{
    state->length = 0U;
    state->overflow = 0U;
}

static uint8_t piTunerSplitSpaces(
    char *line, char **tokens, uint8_t capacity)
{
    uint8_t count = 0U;
    char *cursor = line;

    if (*cursor == '\0') {
        return 0U;
    }

    tokens[count++] = cursor;
    while (*cursor != '\0') {
        if (*cursor == ' ') {
            *cursor = '\0';
            if ((count >= capacity) || (cursor[1] == '\0')) {
                return 0U;
            }
            tokens[count++] = cursor + 1;
        }
        ++cursor;
    }
    return count;
}

static PI_TUNER_ParseResult piTunerParseFixedPointX100(
    const char *text, uint16_t maxX100, uint16_t *value)
{
    uint32_t whole = 0U;
    uint32_t fraction = 0U;
    uint32_t fractionScale = 1U;
    uint32_t scaledFraction;
    uint32_t scaled;
    uint8_t wholeDigits = 0U;
    uint8_t fractionDigits = 0U;

    if ((text == 0) || (value == 0)) {
        return PI_TUNER_PARSE_MALFORMED;
    }
    if (*text == '-') {
        return PI_TUNER_PARSE_OUT_OF_RANGE;
    }

    while ((*text >= '0') && (*text <= '9')) {
        whole = whole * 10U + (uint32_t)(*text - '0');
        if (whole > 65535U) {
            return PI_TUNER_PARSE_OUT_OF_RANGE;
        }
        ++wholeDigits;
        ++text;
    }

    if (wholeDigits == 0U) {
        return PI_TUNER_PARSE_MALFORMED;
    }

    if (*text == '.') {
        ++text;
        while ((*text >= '0') && (*text <= '9')) {
            if (fractionDigits >= 6U) {
                return PI_TUNER_PARSE_MALFORMED;
            }
            fraction = fraction * 10U + (uint32_t)(*text - '0');
            fractionScale *= 10U;
            ++fractionDigits;
            ++text;
        }
        if (fractionDigits == 0U) {
            return PI_TUNER_PARSE_MALFORMED;
        }
    }

    if (*text != '\0') {
        return PI_TUNER_PARSE_MALFORMED;
    }

    if ((maxX100 < 65535U) &&
        (whole >= ((uint32_t)maxX100 / 100U)) &&
        (fraction != 0U)) {
        return PI_TUNER_PARSE_OUT_OF_RANGE;
    }

    scaledFraction =
        (fractionScale == 1U) ? 0U :
        (uint32_t)((fraction * 100U + (fractionScale / 2U)) / fractionScale);
    scaled = whole * 100U + scaledFraction;
    if (scaled > (uint32_t)maxX100) {
        return PI_TUNER_PARSE_OUT_OF_RANGE;
    }

    *value = (uint16_t)scaled;
    return PI_TUNER_PARSE_OK;
}

static uint8_t piTunerParsePrefixedValue(
    const char *token, char prefix, uint16_t maxX100, uint16_t *value,
    PI_TUNER_ParseResult *result)
{
    PI_TUNER_ParseResult parseResult;

    if ((token[0] != prefix) || (token[1] != ':')) {
        if (result != 0) {
            *result = PI_TUNER_PARSE_MALFORMED;
        }
        return 0U;
    }

    parseResult = piTunerParseFixedPointX100(&token[2], maxX100, value);
    if (result != 0) {
        *result = parseResult;
    }
    return (uint8_t)(parseResult == PI_TUNER_PARSE_OK);
}

static const char *piTunerRejectReason(PI_TUNER_ParseResult parseResult)
{
    return (parseResult == PI_TUNER_PARSE_OUT_OF_RANGE) ?
        "gain out of range" : "malformed SET command";
}

static void piTunerApplyTargets(void)
{
    if (gRunning == 0U) {
        WHEEL_SPEED_setTargetsMmps(0, 0);
        TB6612_stopAll();
        return;
    }

    WHEEL_SPEED_setTargetsMmps(0, 0);
}

static void piTunerApplyControlTargets(void)
{
    LINE_FOLLOW_Output lineOutput;

    LINE_FOLLOW_update10ms(TRACK_readRawMask(), &lineOutput);
    WHEEL_SPEED_setTargetsMmps(
        lineOutput.rightTargetMmps, lineOutput.leftTargetMmps);
}

static void piTunerApplyCommonSpeedPid(
    DISPLAY_Source source, uint16_t kpX100, uint16_t kiX100,
    uint16_t kdX100)
{
    uint8_t rightApplied;
    uint8_t leftApplied;
    WHEEL_SPEED_Diagnostics diagnostics;

    DISPLAY_reportPidDecoded(source, kpX100, kiX100, kdX100);
    rightApplied = WHEEL_SPEED_setGainsX100(
        WHEEL_SPEED_WHEEL_RIGHT, kpX100, kiX100);
    leftApplied = WHEEL_SPEED_setGainsX100(
        WHEEL_SPEED_WHEEL_LEFT, kpX100, kiX100);
    if ((rightApplied == 0U) || (leftApplied == 0U)) {
        DISPLAY_reportCommandRejected(source, "gain out of range");
        return;
    }

    WHEEL_SPEED_getDiagnostics(WHEEL_SPEED_WHEEL_RIGHT, &diagnostics);
    DISPLAY_reportPidApplied(
        source, diagnostics.kpX100, diagnostics.kiX100);
}

static void piTunerHandleCommand(
    DISPLAY_Source source, char *line, uint8_t lineOverflow)
{
    char *tokens[5];
    uint8_t count;
    uint16_t kpX100 = 0U;
    uint16_t kiX100 = 0U;
    uint16_t kdX100 = 0U;
    PI_TUNER_ParseResult parseResult = PI_TUNER_PARSE_OK;

    if (lineOverflow != 0U) {
        DISPLAY_reportCommandRejected(source, "line overflow");
        return;
    }

    DISPLAY_reportReceivedLine(source, line);
    count = piTunerSplitSpaces(line, tokens, 5U);
    if ((count != 4U) || (piTunerTextEqual(tokens[0], "SET") == 0U)) {
        DISPLAY_reportCommandRejected(source, "malformed SET command");
        return;
    }

    if ((piTunerParsePrefixedValue(
            tokens[1], 'P', WHEEL_SPEED_KP_X100_MAX, &kpX100,
            &parseResult) == 0U) ||
        (piTunerParsePrefixedValue(
            tokens[2], 'I', WHEEL_SPEED_KI_X100_MAX, &kiX100,
            &parseResult) == 0U) ||
        (piTunerParsePrefixedValue(
            tokens[3], 'D', 65535U, &kdX100, &parseResult) == 0U)) {
        DISPLAY_reportCommandRejected(source, piTunerRejectReason(parseResult));
        return;
    }

    piTunerApplyCommonSpeedPid(source, kpX100, kiX100, kdX100);
}

static void piTunerConsumeByte(
    DISPLAY_Source source, PI_TUNER_LineState *state, uint8_t data)
{
    if (data == (uint8_t)'\r') {
        return;
    }

    if (data == (uint8_t)'\n') {
        if ((state->overflow != 0U) || (state->length != 0U)) {
            state->line[state->length] = '\0';
            piTunerHandleCommand(source, state->line, state->overflow);
        }
        piTunerResetLineState(state);
        return;
    }

    if (state->overflow != 0U) {
        return;
    }

    if (state->length >= (PI_TUNER_LINE_CAPACITY - 1U)) {
        state->overflow = 1U;
        return;
    }

    state->line[state->length++] = (char)data;
}

static uint8_t piTunerHandleDebugControlLine(
    char *line, uint8_t lineOverflow)
{
    if (lineOverflow != 0U) {
        DISPLAY_reportCommandRejected(
            DISPLAY_SOURCE_DEBUG_UART, "debug line overflow");
        return 1U;
    }

    if ((piTunerTextEqual(line, "menu") != 0U) ||
        (piTunerTextEqual(line, "m") != 0U) ||
        (piTunerTextEqual(line, "0") != 0U)) {
        piTunerSelectDebugMode(PI_TUNER_DEBUG_MODE_NONE);
        piTunerPrintDebugMenu();
        return 1U;
    }

    if ((piTunerTextEqual(line, "1") != 0U) ||
        (piTunerTextEqual(line, "mode1") != 0U)) {
        piTunerSelectDebugMode(PI_TUNER_DEBUG_MODE_PWM);
        DEBUG_UART_writeString("[DBG MODE] mode1 PWM 50ms\r\n");
        return 1U;
    }

    if ((piTunerTextEqual(line, "2") != 0U) ||
        (piTunerTextEqual(line, "mode2") != 0U)) {
        gImuInitAttempted = 0U;
        piTunerSelectDebugMode(PI_TUNER_DEBUG_MODE_IMU);
        DEBUG_UART_writeString("[DBG MODE] mode2 IMU 20ms\r\n");
        (void)piTunerEnsureImuInitialized(1U);
        return 1U;
    }

    if ((piTunerTextEqual(line, "3") != 0U) ||
        (piTunerTextEqual(line, "mode3") != 0U)) {
        piTunerSelectDebugMode(PI_TUNER_DEBUG_MODE_PID);
        DEBUG_UART_writeString("[DBG MODE] mode3 PID 500ms\r\n");
        return 1U;
    }

    return 0U;
}

static uint8_t piTunerHandleDebugDigitCommand(uint8_t data)
{
    switch (data) {
        case (uint8_t)'0':
            piTunerSelectDebugMode(PI_TUNER_DEBUG_MODE_NONE);
            piTunerPrintDebugMenu();
            return 1U;
        case (uint8_t)'1':
            piTunerSelectDebugMode(PI_TUNER_DEBUG_MODE_PWM);
            DEBUG_UART_writeString("[DBG MODE] mode1 PWM 50ms\r\n");
            return 1U;
        case (uint8_t)'2':
            gImuInitAttempted = 0U;
            piTunerSelectDebugMode(PI_TUNER_DEBUG_MODE_IMU);
            DEBUG_UART_writeString("[DBG MODE] mode2 IMU 20ms\r\n");
            (void)piTunerEnsureImuInitialized(1U);
            return 1U;
        case (uint8_t)'3':
            piTunerSelectDebugMode(PI_TUNER_DEBUG_MODE_PID);
            DEBUG_UART_writeString("[DBG MODE] mode3 PID 500ms\r\n");
            return 1U;
        default:
            break;
    }

    return 0U;
}

static void piTunerCycleDebugModeFromButton(void)
{
    uint8_t command;

    switch (gDebugMode) {
        case PI_TUNER_DEBUG_MODE_PWM:
            command = (uint8_t)'2';
            break;
        case PI_TUNER_DEBUG_MODE_IMU:
            command = (uint8_t)'3';
            break;
        case PI_TUNER_DEBUG_MODE_PID:
            command = (uint8_t)'1';
            break;
        default:
            command = (uint8_t)'1';
            break;
    }

    (void)piTunerHandleDebugDigitCommand(command);
}

static void piTunerScanDebugModeButton(void)
{
    uint8_t stableMask;

    if ((KEY_scan10ms(&stableMask) != false) &&
        ((stableMask & KEY_MASK_KEY3) != 0U)) {
        piTunerCycleDebugModeFromButton();
    }
}

static void piTunerConsumeDebugByte(uint8_t data)
{
    PI_TUNER_LineState *state = &gDebugLineState;

    if ((state->length == 0U) && (state->overflow == 0U) &&
        (piTunerHandleDebugDigitCommand(data) != 0U)) {
        return;
    }

    if (data == (uint8_t)'\r') {
        return;
    }

    if (data == (uint8_t)'\n') {
        if ((state->overflow != 0U) || (state->length != 0U)) {
            state->line[state->length] = '\0';
            if (piTunerHandleDebugControlLine(
                    state->line, state->overflow) == 0U) {
                piTunerHandleCommand(
                    DISPLAY_SOURCE_DEBUG_UART,
                    state->line, state->overflow);
            }
        }
        piTunerResetLineState(state);
        return;
    }

    if (state->overflow != 0U) {
        return;
    }

    if (state->length >= (PI_TUNER_LINE_CAPACITY - 1U)) {
        state->overflow = 1U;
        return;
    }

    state->line[state->length++] = (char)data;
}

static void piTunerHandleBtPacket(const uint8_t *frame, uint8_t frameLength)
{
    PI_TUNER_FloatPid innerPid;
    PI_TUNER_FloatPid anglePid;
    PI_TUNER_FloatPid linePid;
    int16_t targetMmps;
    float targetYawDeg;
    uint8_t run;
    uint8_t wasRunning;
    uint16_t kpX100;
    uint16_t kiX100;
    uint16_t kdX100;

    innerPid = piTunerReadFloatPid(frame, PI_TUNER_BT_INNER_PID_OFFSET);
    if ((piTunerFloatToX100(
            innerPid.p, WHEEL_SPEED_KP_X100_MAX, &kpX100) == 0U) ||
        (piTunerFloatToX100(
            innerPid.i, WHEEL_SPEED_KI_X100_MAX, &kiX100) == 0U) ||
        (piTunerFloatToX100(innerPid.d, 65535U, &kdX100) == 0U)) {
        DISPLAY_reportCommandRejected(
            DISPLAY_SOURCE_BLUETOOTH, "binary PID out of range");
        return;
    }

    anglePid = piTunerReadFloatPid(frame, PI_TUNER_BT_ANGLE_PID_OFFSET);
    linePid = piTunerReadFloatPid(frame, PI_TUNER_BT_LINE_PID_OFFSET);
    targetYawDeg = (frameLength >= PI_TUNER_BT_FRAME_LEN) ?
        piTunerReadFloatLe(frame, PI_TUNER_BT_TARGET_YAW_OFFSET) :
        gTargetYawDeg;
    targetMmps = piTunerClampTargetInt32(
        piTunerReadI32Le(frame, PI_TUNER_BT_SPEED_OFFSET));
    run = (frame[PI_TUNER_BT_RUN_OFFSET] != 0U) ? 1U : 0U;

    if ((kdX100 != 0U) ||
        (piTunerPidIsFiniteNonnegative(anglePid) == 0U) ||
        (LINE_FOLLOW_pidIsValid(
            linePid.p, linePid.i, linePid.d) == 0U) ||
        (piTunerFloatIsFinite(targetYawDeg) == 0U)) {
        DISPLAY_reportCommandRejected(
            DISPLAY_SOURCE_BLUETOOTH, "binary PID out of range");
        return;
    }

    wasRunning = gRunning;
    gOuterAnglePid = anglePid;
    gOuterLinePid = linePid;
    gTargetMmps = targetMmps;
    ANGLE_CONTROL_setBaseSpeedMmps(targetMmps);
    ANGLE_CONTROL_setPid(anglePid.p, anglePid.i, anglePid.d);
    gTargetYawDeg = targetYawDeg;

    if ((run != 0U) && (wasRunning == 0U)) {
        LINE_FOLLOW_init();
    }
    LINE_FOLLOW_setBaseSpeedMmps(targetMmps);
    (void)LINE_FOLLOW_setPid(linePid.p, linePid.i, linePid.d);

    piTunerReportBtPacketDecoded(
        run, targetMmps, innerPid, anglePid, linePid, targetYawDeg);

    piTunerApplyCommonSpeedPid(
        DISPLAY_SOURCE_BLUETOOTH, kpX100, kiX100, kdX100);

    gRunning = run;
    if (gRunning == 0U) {
        ANGLE_CONTROL_disable();
        piTunerApplyTargets();
        return;
    }

    ANGLE_CONTROL_disable();
    if (wasRunning == 0U) {
        WHEEL_SPEED_setTargetsMmps(0, 0);
        TB6612_stopAll();
    }
}

static void piTunerResetBtPacketState(void)
{
    gBtPacketState.length = 0U;
}

static void piTunerResyncBtPacketState(void)
{
    uint8_t start;
    uint8_t i;

    for (start = 1U; start < gBtPacketState.length; ++start) {
        if (gBtPacketState.frame[start] == PI_TUNER_BT_PACKET_HEAD) {
            uint8_t newLength =
                (uint8_t)(gBtPacketState.length - start);
            for (i = 0U; i < newLength; ++i) {
                gBtPacketState.frame[i] =
                    gBtPacketState.frame[start + i];
            }
            gBtPacketState.length = newLength;
            return;
        }
    }

    piTunerResetBtPacketState();
}

static void piTunerFinishBtPacket(void)
{
    if (piTunerBtPacketIsValid(
            gBtPacketState.frame, gBtPacketState.length) != 0U) {
        piTunerHandleBtPacket(gBtPacketState.frame, gBtPacketState.length);
        piTunerResetBtPacketState();
        return;
    }

    if ((gBtPacketState.length >= PI_TUNER_BT_FRAME_LEN) &&
        (gBtPacketState.frame[PI_TUNER_BT_FRAME_LEN - 1U] ==
            PI_TUNER_BT_PACKET_TAIL)) {
        DISPLAY_reportCommandRejected(
            DISPLAY_SOURCE_BLUETOOTH, "binary packet checksum");
    }
    piTunerResyncBtPacketState();
}

static void piTunerConsumeBluetoothByte(uint8_t data)
{
    if ((gBtPacketState.length == 0U) &&
        (data != PI_TUNER_BT_PACKET_HEAD)) {
        piTunerConsumeByte(
            DISPLAY_SOURCE_BLUETOOTH, &gBtLineState, data);
        return;
    }

    if (gBtPacketState.length == 0U) {
        gBtPacketState.frame[0] = data;
        gBtPacketState.length = 1U;
        return;
    }

    gBtPacketState.frame[gBtPacketState.length] = data;
    ++gBtPacketState.length;

    if ((gBtPacketState.length == PI_TUNER_BT_LEGACY_FRAME_LEN) &&
        (piTunerBtPacketIsValid(
            gBtPacketState.frame, PI_TUNER_BT_LEGACY_FRAME_LEN) != 0U)) {
        piTunerFinishBtPacket();
        return;
    }

    if (gBtPacketState.length >= PI_TUNER_BT_FRAME_LEN) {
        piTunerFinishBtPacket();
    }
}

void PI_TUNER_init(void)
{
    WHEEL_SPEED_init();
    ANGLE_CONTROL_init();
    LINE_FOLLOW_init();
    KEY_init();
    gTargetMmps = PI_TUNER_FIXED_TARGET_MMPS;
    gTargetYawDeg = 0.0f;
    ANGLE_CONTROL_setBaseSpeedMmps(gTargetMmps);
    LINE_FOLLOW_setBaseSpeedMmps(gTargetMmps);
    gRunning = 1U;
    piTunerApplyTargets();
    TB6612_stopAll();
    piTunerSelectDebugMode(PI_TUNER_DEBUG_MODE_NONE);
#if PI_TUNER_ENABLE_BT_TELEMETRY
    gTelemetryStarted = 0U;
    gLastTelemetryTick = 0U;
#endif
    piTunerResetLineState(&gBtLineState);
    piTunerResetLineState(&gDebugLineState);
    piTunerResetBtPacketState();
    gOuterAnglePid.p = 0.0f;
    gOuterAnglePid.i = 0.0f;
    gOuterAnglePid.d = 0.0f;
    gOuterLinePid.p = LINE_FOLLOW_DEFAULT_KP;
    gOuterLinePid.i = LINE_FOLLOW_DEFAULT_KI;
    gOuterLinePid.d = LINE_FOLLOW_DEFAULT_KD;
    gImuInitialized = 0U;
    gImuInitAttempted = 0U;
    piTunerPrintDebugMenu();
}

void PI_TUNER_pollUart(void)
{
    uint8_t data;

    while (BT_UART_tryReadByte(&data) != 0U) {
        piTunerConsumeBluetoothByte(data);
    }

    while (DEBUG_UART_tryReadByte(&data) != 0U) {
        piTunerConsumeDebugByte(data);
    }
}

void PI_TUNER_update10ms(uint32_t tick10ms)
{
    int32_t rightCounts10ms;
    int32_t leftCounts10ms;

    piTunerScanDebugModeButton();

    if (gRunning == 0U) {
        piTunerServiceDebugMode(tick10ms);
        return;
    }

    rightCounts10ms = ENCODER_getRightSpeed10ms();
    leftCounts10ms = ENCODER_getLeftSpeed10ms();
    piTunerApplyControlTargets();
    WHEEL_SPEED_update10ms(rightCounts10ms, leftCounts10ms);
    piTunerApplyMotorOutputs();

#if PI_TUNER_ENABLE_BT_TELEMETRY
    if ((gTelemetryStarted == 0U) ||
        ((uint32_t)(tick10ms - gLastTelemetryTick) >=
            PI_TUNER_TELEMETRY_TICKS)) {
        PI_TUNER_pollUart();
        gTelemetryStarted = 1U;
        gLastTelemetryTick = tick10ms;
        piTunerWriteTelemetry();
    }
#endif

    piTunerServiceDebugMode(tick10ms);
}

uint8_t PI_TUNER_isRunning(void)
{
    return gRunning;
}
