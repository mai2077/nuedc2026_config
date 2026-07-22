#include "pi_tuner.h"

#include "bt_uart.h"
#include "debug_uart.h"
#include "encoder.h"
#include "tb6612.h"

#define PI_TUNER_LINE_CAPACITY       (64U)
#define PI_TUNER_TELEMETRY_TICKS     (2U)
#define PI_TUNER_PROFILE_STEP_TICKS  (200U)
#define PI_TUNER_PROFILE_TOTAL_TICKS (1000U)

static const int16_t gProfileMmps[] = {300, 500, 700, 500, 300};

static uint8_t gRunning;
#if PI_TUNER_ENABLE_RESV1_TELEMETRY
static uint8_t gTelemetryStarted;
static uint32_t gLastTelemetryTick;
#endif
static char gLine[PI_TUNER_LINE_CAPACITY];
static uint8_t gLineLength;
static uint8_t gLineOverflow;

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

static int16_t piTunerProfileTarget(uint32_t tick10ms)
{
    uint32_t elapsedTick = (tick10ms == 0U) ? 0U : tick10ms - 1U;
    uint32_t profileTick = elapsedTick % PI_TUNER_PROFILE_TOTAL_TICKS;
    uint32_t index = profileTick / PI_TUNER_PROFILE_STEP_TICKS;

    return gProfileMmps[index];
}

static void piTunerApplyMotorOutputs(void)
{
    TB6612_setMotor1((int16_t)-WHEEL_SPEED_getRightPwm());
    TB6612_setMotor2(0);
}

#if PI_TUNER_ENABLE_RESV1_TELEMETRY
static void piTunerWriteFixed2Integer(int32_t value)
{
    DEBUG_UART_writeInt32(value);
    DEBUG_UART_writeString(".00");
}

static void piTunerWriteTelemetry(uint32_t tick10ms)
{
    WHEEL_SPEED_Diagnostics diagnostics;

    WHEEL_SPEED_getDiagnostics(WHEEL_SPEED_WHEEL_RIGHT, &diagnostics);
    DEBUG_UART_writeUInt32(tick10ms * 10U);
    DEBUG_UART_writeByte((uint8_t)',');
    piTunerWriteFixed2Integer(diagnostics.setpointMmps);
    DEBUG_UART_writeByte((uint8_t)',');
    piTunerWriteFixed2Integer(diagnostics.inputMmps);
    DEBUG_UART_writeByte((uint8_t)',');
    piTunerWriteFixed2Integer(diagnostics.outputPwm);
    DEBUG_UART_writeByte((uint8_t)',');
    piTunerWriteFixed2Integer(diagnostics.errorMmps);
    DEBUG_UART_writeByte((uint8_t)',');
    DEBUG_UART_writeSignedFixed3((int32_t)diagnostics.kpX100 * 10);
    DEBUG_UART_writeByte((uint8_t)',');
    DEBUG_UART_writeSignedFixed3((int32_t)diagnostics.kiX100 * 10);
    DEBUG_UART_writeString(",0.000\r\n");
}
#endif

static uint8_t piTunerParseX100(const char *text, uint16_t *value)
{
    uint32_t whole = 0U;
    uint32_t fraction = 0U;
    uint32_t scaled;
    uint8_t wholeDigits = 0U;
    uint8_t fractionDigits = 0U;

    if ((text == 0) || (value == 0)) {
        return 0U;
    }

    while ((*text >= '0') && (*text <= '9')) {
        whole = whole * 10U + (uint32_t)(*text - '0');
        if (whole > 655U) {
            return 0U;
        }
        ++wholeDigits;
        ++text;
    }

    if (*text == '.') {
        ++text;
        while ((*text >= '0') && (*text <= '9')) {
            if (fractionDigits >= 2U) {
                return 0U;
            }
            fraction = fraction * 10U + (uint32_t)(*text - '0');
            ++fractionDigits;
            ++text;
        }
        if (fractionDigits == 0U) {
            return 0U;
        }
    }

    if ((wholeDigits == 0U) || (*text != '\0')) {
        return 0U;
    }
    if (fractionDigits == 1U) {
        fraction *= 10U;
    }

    scaled = whole * 100U + fraction;
    if (scaled > 65535U) {
        return 0U;
    }
    *value = (uint16_t)scaled;
    return 1U;
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

static uint8_t piTunerParsePrefixed(
    const char *token, char prefix, uint16_t *value)
{
    if ((token[0] != prefix) || (token[1] != ':')) {
        return 0U;
    }
    return piTunerParseX100(&token[2], value);
}

static void piTunerHandleCommand(char *line)
{
    char *tokens[5];
    uint8_t count = piTunerSplitSpaces(line, tokens, 5U);
    uint16_t kpX100;
    uint16_t kiX100;
    uint16_t kdX100;

    if ((count != 4U) ||
        (piTunerTextEqual(tokens[0], "SET") == 0U) ||
        (piTunerParsePrefixed(tokens[1], 'P', &kpX100) == 0U) ||
        (piTunerParsePrefixed(tokens[2], 'I', &kiX100) == 0U) ||
        (piTunerParsePrefixed(tokens[3], 'D', &kdX100) == 0U) ||
        (kdX100 != 0U)) {
        return;
    }

    (void)WHEEL_SPEED_setGainsX100(
        WHEEL_SPEED_WHEEL_RIGHT, kpX100, kiX100);
}

void PI_TUNER_init(void)
{
    WHEEL_SPEED_init();
    WHEEL_SPEED_setTargetsMmps(300, 0);
    TB6612_stopAll();
    gRunning = 1U;
#if PI_TUNER_ENABLE_RESV1_TELEMETRY
    gTelemetryStarted = 0U;
    gLastTelemetryTick = 0U;
#endif
    gLineLength = 0U;
    gLineOverflow = 0U;
}

void PI_TUNER_pollUart(void)
{
    uint8_t data;

    while (DEBUG_UART_tryReadByte(&data) != 0U) {
        BT_UART_writeByte(data);
        if (data == (uint8_t)'\r') {
            continue;
        }
        if (data == (uint8_t)'\n') {
            if ((gLineOverflow == 0U) && (gLineLength != 0U)) {
                gLine[gLineLength] = '\0';
                piTunerHandleCommand(gLine);
            }
            gLineLength = 0U;
            gLineOverflow = 0U;
            continue;
        }
        if (gLineOverflow != 0U) {
            continue;
        }
        if (gLineLength >= (PI_TUNER_LINE_CAPACITY - 1U)) {
            gLineOverflow = 1U;
            continue;
        }
        gLine[gLineLength++] = (char)data;
    }
}

void PI_TUNER_update10ms(uint32_t tick10ms)
{
    int16_t targetMmps;

    if (gRunning == 0U) {
        return;
    }

    targetMmps = piTunerProfileTarget(tick10ms);
    WHEEL_SPEED_setTargetsMmps(targetMmps, 0);
    WHEEL_SPEED_update10ms(
        ENCODER_getRightSpeed10ms(), ENCODER_getLeftSpeed10ms());
    piTunerApplyMotorOutputs();

#if PI_TUNER_ENABLE_RESV1_TELEMETRY
    if ((gTelemetryStarted == 0U) ||
        ((uint32_t)(tick10ms - gLastTelemetryTick) >=
            PI_TUNER_TELEMETRY_TICKS)) {
        gTelemetryStarted = 1U;
        gLastTelemetryTick = tick10ms;
        piTunerWriteTelemetry(tick10ms);
    }
#endif
}

uint8_t PI_TUNER_isRunning(void)
{
    return gRunning;
}
