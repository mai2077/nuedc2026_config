#ifdef PI_TUNER_HOST_TEST

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "pi_tuner.h"
#include "wheel_speed.h"

static int32_t gRightCounts10ms;
static int32_t gLeftCounts10ms;
static int16_t gMotor1Command;
static int16_t gMotor2Command;
static char gTx[32768];
static size_t gTxLength;
static char gBtTx[256];
static size_t gBtTxLength;
static char gRx[256];
static size_t gRxLength;
static size_t gRxIndex;

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

static void appendText(const char *text)
{
    size_t length = strlen(text);
    assert(gTxLength + length < sizeof(gTx));
    memcpy(&gTx[gTxLength], text, length + 1U);
    gTxLength += length;
}

void DEBUG_UART_writeString(const char *text)
{
    appendText(text);
}

void DEBUG_UART_writeByte(uint8_t data)
{
    char text[2];
    text[0] = (char)data;
    text[1] = '\0';
    appendText(text);
}

void DEBUG_UART_writeUInt32(uint32_t value)
{
    char text[16];
    (void)snprintf(text, sizeof(text), "%lu", (unsigned long)value);
    appendText(text);
}

void DEBUG_UART_writeInt32(int32_t value)
{
    char text[16];
    (void)snprintf(text, sizeof(text), "%ld", (long)value);
    appendText(text);
}

void DEBUG_UART_writeSignedFixed3(int32_t milliValue)
{
    char text[24];
    uint32_t magnitude = (milliValue < 0) ?
        ((uint32_t)(-(milliValue + 1)) + 1U) : (uint32_t)milliValue;

    (void)snprintf(text, sizeof(text), "%s%lu.%03lu",
        (milliValue < 0) ? "-" : "",
        (unsigned long)(magnitude / 1000U),
        (unsigned long)(magnitude % 1000U));
    appendText(text);
}

void BT_UART_writeByte(uint8_t data)
{
    assert(gBtTxLength + 1U < sizeof(gBtTx));
    gBtTx[gBtTxLength++] = (char)data;
    gBtTx[gBtTxLength] = '\0';
}

uint8_t DEBUG_UART_tryReadByte(uint8_t *data)
{
    if (gRxIndex >= gRxLength) {
        return 0U;
    }
    *data = (uint8_t)gRx[gRxIndex++];
    return 1U;
}

static void clearTx(void)
{
    gTxLength = 0U;
    gTx[0] = '\0';
}

static void clearBtTx(void)
{
    gBtTxLength = 0U;
    gBtTx[0] = '\0';
}

static void feedCommand(const char *command)
{
    size_t length = strlen(command);
    assert(length < sizeof(gRx));
    memcpy(gRx, command, length);
    gRxLength = length;
    gRxIndex = 0U;
    PI_TUNER_pollUart();
}

static void resetModel(void)
{
    gRightCounts10ms = 0;
    gLeftCounts10ms = 0;
    gMotor1Command = 0;
    gMotor2Command = 0;
    clearBtTx();
    gRxLength = 0U;
    gRxIndex = 0U;
    clearTx();
    PI_TUNER_init();
    clearTx();
}

static void testStartsAutomaticallyAndOutputsOnlyNumericCsv(void)
{
    resetModel();
    assert(PI_TUNER_isRunning() != 0U);

    PI_TUNER_update10ms(1U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 300);
    assert(WHEEL_SPEED_getLeftTargetMmps() == 0);
    assert(gMotor1Command < 0);
    assert(gMotor2Command == 0);
#if PI_TUNER_ENABLE_RESV1_TELEMETRY
    assert(strcmp(gTx,
        "10,300.00,0.00,105.00,300.00,20.000,0.000,0.000\r\n") == 0);
    assert(strchr(gTx, '#') == 0);
#else
    assert(gTx[0] == '\0');
#endif
}

static void testProfileRepeatsEveryTenSeconds(void)
{
    resetModel();
    PI_TUNER_update10ms(1U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 300);
    PI_TUNER_update10ms(201U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 500);
    PI_TUNER_update10ms(401U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 700);
    PI_TUNER_update10ms(601U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 500);
    PI_TUNER_update10ms(801U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 300);
    PI_TUNER_update10ms(1001U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 300);
    assert(PI_TUNER_isRunning() != 0U);
    assert(gMotor1Command < 0);
    assert(gMotor2Command == 0);
}

#if PI_TUNER_ENABLE_RESV1_TELEMETRY
static void testTelemetryPeriodIsTwentyMilliseconds(void)
{
    resetModel();
    PI_TUNER_update10ms(1U);
    assert(gTxLength != 0U);
    clearTx();
    PI_TUNER_update10ms(2U);
    assert(gTxLength == 0U);
    PI_TUNER_update10ms(3U);
    assert(gTxLength != 0U);
}
#endif

static void testSetCommandUpdatesPiSilently(void)
{
    WHEEL_SPEED_Diagnostics diagnostics;

    resetModel();
    feedCommand("SET P:20.50 I:1.75 D:0\n");
    WHEEL_SPEED_getDiagnostics(WHEEL_SPEED_WHEEL_RIGHT, &diagnostics);
    assert(diagnostics.kpX100 == 2050U);
    assert(diagnostics.kiX100 == 175U);
    assert(gTx[0] == '\0');

    PI_TUNER_update10ms(1U);
#if PI_TUNER_ENABLE_RESV1_TELEMETRY
    assert(strstr(gTx, ",20.500,1.750,0.000\r\n") != 0);
#else
    assert(gTx[0] == '\0');
#endif
}

static void testIntegerSetCommandIsAccepted(void)
{
    WHEEL_SPEED_Diagnostics diagnostics;

    resetModel();
    feedCommand("SET P:20 I:2 D:0\r\n");
    WHEEL_SPEED_getDiagnostics(WHEEL_SPEED_WHEEL_RIGHT, &diagnostics);
    assert(diagnostics.kpX100 == 2000U);
    assert(diagnostics.kiX100 == 200U);
    assert(gTx[0] == '\0');
}

static void assertInvalidCommandLeavesGainsUnchanged(const char *command)
{
    WHEEL_SPEED_Diagnostics before;
    WHEEL_SPEED_Diagnostics after;

    WHEEL_SPEED_getDiagnostics(WHEEL_SPEED_WHEEL_RIGHT, &before);
    feedCommand(command);
    WHEEL_SPEED_getDiagnostics(WHEEL_SPEED_WHEEL_RIGHT, &after);
    assert(after.kpX100 == before.kpX100);
    assert(after.kiX100 == before.kiX100);
    assert(gTx[0] == '\0');
}

static void testInvalidCommandsAreIgnoredSilently(void)
{
    resetModel();
    assertInvalidCommandLeavesGainsUnchanged("SET P:20 I:2 D:1\n");
    assertInvalidCommandLeavesGainsUnchanged("SET P:100.01 I:2 D:0\n");
    assertInvalidCommandLeavesGainsUnchanged("SET P:20 I:20.01 D:0\n");
    assertInvalidCommandLeavesGainsUnchanged("SET P:-1 I:2 D:0\n");
    assertInvalidCommandLeavesGainsUnchanged("SET P:1.234 I:2 D:0\n");
    assertInvalidCommandLeavesGainsUnchanged("garbage\n");
}

static void testOverlongLineIsDiscardedAndNextLineRecovers(void)
{
    WHEEL_SPEED_Diagnostics diagnostics;
    char command[100];

    resetModel();
    memset(command, 'X', sizeof(command));
    command[sizeof(command) - 2U] = '\n';
    command[sizeof(command) - 1U] = '\0';
    feedCommand(command);
    feedCommand("SET P:12.25 I:0.50 D:0\n");
    WHEEL_SPEED_getDiagnostics(WHEEL_SPEED_WHEEL_RIGHT, &diagnostics);
    assert(diagnostics.kpX100 == 1225U);
    assert(diagnostics.kiX100 == 50U);
    assert(gTx[0] == '\0');
}

static void testGainUpdateResetsPiOutput(void)
{
    WHEEL_SPEED_Diagnostics diagnostics;

    resetModel();
    PI_TUNER_update10ms(1U);
    assert(WHEEL_SPEED_getRightPwm() != 0);

    clearTx();
    feedCommand("SET P:10 I:1 D:0\n");
    WHEEL_SPEED_getDiagnostics(WHEEL_SPEED_WHEEL_RIGHT, &diagnostics);
    assert(diagnostics.outputPwm == 0);
    assert(diagnostics.integralPwm == 0);
    assert(gTx[0] == '\0');
}

static void testResv1BytesAreForwardedUnchangedToBluetooth(void)
{
    const char command[] = "SET P:20.50 I:1.75 D:0\r\nX  !\n";

    resetModel();
    feedCommand(command);
    assert(strcmp(gBtTx, command) == 0);
}

int main(void)
{
    testStartsAutomaticallyAndOutputsOnlyNumericCsv();
    testProfileRepeatsEveryTenSeconds();
#if PI_TUNER_ENABLE_RESV1_TELEMETRY
    testTelemetryPeriodIsTwentyMilliseconds();
#endif
    testSetCommandUpdatesPiSilently();
    testIntegerSetCommandIsAccepted();
    testInvalidCommandsAreIgnoredSilently();
    testOverlongLineIsDiscardedAndNextLineRecovers();
    testGainUpdateResetsPiOutput();
    testResv1BytesAreForwardedUnchangedToBluetooth();
    puts("pi tuner tests passed");
    return 0;
}

#endif /* PI_TUNER_HOST_TEST */
