#ifdef ANO_PROTOCOL_HOST_TEST

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ano_protocol.h"

#define TEST_PI (3.14159265358979323846f)

static uint8_t gTxBytes[64];
static uint16_t gTxCount;

void DEBUG_UART_writeByte(uint8_t data)
{
    assert(gTxCount < sizeof(gTxBytes));
    gTxBytes[gTxCount++] = data;
}

static int16_t readS16(const uint8_t *bytes)
{
    return (int16_t)((uint16_t)bytes[0] |
                     ((uint16_t)bytes[1] << 8U));
}

static void verifyChecksums(const uint8_t *frame, uint16_t length)
{
    uint8_t sum = 0U;
    uint8_t add = 0U;
    uint16_t index;

    assert(length == 21U);
    for (index = 0U; index < 19U; ++index) {
        sum = (uint8_t)(sum + frame[index]);
        add = (uint8_t)(add + sum);
    }

    assert(frame[19] == sum);
    assert(frame[20] == add);
}

static void testFixedImuFrame(void)
{
    IMU_DATA_t sample = {0};
    static const uint8_t prefix[] = {
        0xABU, 0xDDU, 0xFEU, 0x01U, 0x0DU, 0x00U};

    memset(gTxBytes, 0, sizeof(gTxBytes));
    gTxCount = 0U;
    sample.gyro.z = 1000.0f * TEST_PI / 180.0f;

    ANO_sendImuFrame(&sample);

    assert(gTxCount == 21U);
    assert(memcmp(gTxBytes, prefix, sizeof(prefix)) == 0);
    assert(readS16(&gTxBytes[6]) == 0);
    assert(readS16(&gTxBytes[8]) == 0);
    assert(readS16(&gTxBytes[10]) == 0);
    assert(readS16(&gTxBytes[12]) == 0);
    assert(readS16(&gTxBytes[14]) == 0);
    assert(readS16(&gTxBytes[16]) == 16384);
    assert(gTxBytes[18] == 0U);
    verifyChecksums(gTxBytes, gTxCount);
}

static void testConversionsAndSaturation(void)
{
    assert(ANO_accelToCmss(1.23f) == 123);
    assert(ANO_accelToCmss(-1.23f) == -123);
    assert(ANO_accelToCmss(400.0f) == INT16_MAX);
    assert(ANO_accelToCmss(-400.0f) == INT16_MIN);

    assert(ANO_gyroToCounts(1000.0f * TEST_PI / 180.0f) == 16384);
    assert(ANO_gyroToCounts(-1000.0f * TEST_PI / 180.0f) == -16384);
    assert(ANO_gyroToCounts(4000.0f * TEST_PI / 180.0f) == INT16_MAX);
    assert(ANO_gyroToCounts(-4000.0f * TEST_PI / 180.0f) == INT16_MIN);
    assert(ANO_accelToCmss(NAN) == 0);
    assert(ANO_gyroToCounts(INFINITY) == 0);
    assert(ANO_gyroToCounts(-INFINITY) == 0);
}

static void testNegativePayloadAndNullInput(void)
{
    IMU_DATA_t sample = {0};

    sample.accel.x = -9.81f;
    sample.gyro.y = -1000.0f * TEST_PI / 180.0f;
    gTxCount = 0U;
    ANO_sendImuFrame(&sample);

    assert(gTxCount == 21U);
    assert(readS16(&gTxBytes[6]) == -981);
    assert(readS16(&gTxBytes[14]) == -16384);
    verifyChecksums(gTxBytes, gTxCount);

    gTxCount = 0U;
    ANO_sendImuFrame(NULL);
    assert(gTxCount == 0U);
}

int main(void)
{
    testFixedImuFrame();
    testConversionsAndSaturation();
    testNegativePayloadAndNullInput();
    puts("ano protocol tests passed");
    return 0;
}

#endif /* ANO_PROTOCOL_HOST_TEST */
