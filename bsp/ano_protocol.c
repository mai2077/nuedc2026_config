#include "ano_protocol.h"

#include <math.h>
#include <stddef.h>

#include "debug_uart.h"

#define ANO_IMU_FRAME_LENGTH (21U)
#define ANO_IMU_DATA_LENGTH (13U)
#define ANO_EULER_FRAME_LENGTH (15U)
#define ANO_EULER_DATA_LENGTH (7U)
#define ANO_HEAD (0xABU)
#define ANO_SOURCE_ADDRESS (0xDDU)
#define ANO_DESTINATION_ADDRESS (0xFEU)
#define ANO_FRAME_ID_IMU (0x01U)
#define ANO_FRAME_ID_EULER (0x03U)
#define ANO_PI (3.14159265358979323846f)
#define ANO_GYRO_COUNTS_PER_DPS (16.384f)

static int16_t ANO_clampS16(float value)
{
    int32_t rounded;

    if (!isfinite(value)) {
        return 0;
    }
    if (value >= (float)INT16_MAX) {
        return INT16_MAX;
    }
    if (value <= (float)INT16_MIN) {
        return INT16_MIN;
    }

    rounded = (value >= 0.0f) ? (int32_t)(value + 0.5f) :
                                (int32_t)(value - 0.5f);
    if (rounded > INT16_MAX) {
        rounded = INT16_MAX;
    } else if (rounded < INT16_MIN) {
        rounded = INT16_MIN;
    }

    return (int16_t)rounded;
}

static void ANO_putS16LE(uint8_t *frame, uint16_t offset, int16_t value)
{
    uint16_t raw = (uint16_t)value;

    frame[offset] = (uint8_t)(raw & 0xFFU);
    frame[offset + 1U] = (uint8_t)(raw >> 8U);
}

int16_t ANO_accelToCmss(float value)
{
    return ANO_clampS16(value * 100.0f);
}

int16_t ANO_gyroToCounts(float value)
{
    float degreesPerSecond = value * (180.0f / ANO_PI);

    return ANO_clampS16(degreesPerSecond * ANO_GYRO_COUNTS_PER_DPS);
}

static int16_t ANO_angleRadToCentideg(float value)
{
    float degrees = value * (180.0f / ANO_PI);

    return ANO_clampS16(degrees * 100.0f);
}

void ANO_sendImuFrame(const IMU_DATA_t *sample)
{
    uint8_t frame[ANO_IMU_FRAME_LENGTH];
    uint8_t sum = 0U;
    uint8_t add = 0U;
    uint16_t index;

    if (NULL == sample) {
        return;
    }

    frame[0] = ANO_HEAD;
    frame[1] = ANO_SOURCE_ADDRESS;
    frame[2] = ANO_DESTINATION_ADDRESS;
    frame[3] = ANO_FRAME_ID_IMU;
    frame[4] = ANO_IMU_DATA_LENGTH;
    frame[5] = 0U;
    ANO_putS16LE(frame, 6U, ANO_accelToCmss(sample->accel.x));
    ANO_putS16LE(frame, 8U, ANO_accelToCmss(sample->accel.y));
    ANO_putS16LE(frame, 10U, ANO_accelToCmss(sample->accel.z));
    ANO_putS16LE(frame, 12U, ANO_gyroToCounts(sample->gyro.x));
    ANO_putS16LE(frame, 14U, ANO_gyroToCounts(sample->gyro.y));
    ANO_putS16LE(frame, 16U, ANO_gyroToCounts(sample->gyro.z));
    frame[18] = 0U;

    for (index = 0U; index < (ANO_IMU_FRAME_LENGTH - 2U); ++index) {
        sum = (uint8_t)(sum + frame[index]);
        add = (uint8_t)(add + sum);
    }
    frame[19] = sum;
    frame[20] = add;

    for (index = 0U; index < ANO_IMU_FRAME_LENGTH; ++index) {
        DEBUG_UART_writeByte(frame[index]);
    }
}

void ANO_sendEulerFrame(
    const ATTITUDE_DATA_t *attitude, uint8_t fusionStatus)
{
    uint8_t frame[ANO_EULER_FRAME_LENGTH];
    uint8_t sum = 0U;
    uint8_t add = 0U;
    uint16_t index;

    if (NULL == attitude) {
        return;
    }

    frame[0] = ANO_HEAD;
    frame[1] = ANO_SOURCE_ADDRESS;
    frame[2] = ANO_DESTINATION_ADDRESS;
    frame[3] = ANO_FRAME_ID_EULER;
    frame[4] = ANO_EULER_DATA_LENGTH;
    frame[5] = 0U;
    ANO_putS16LE(frame, 6U, ANO_angleRadToCentideg(attitude->roll));
    ANO_putS16LE(frame, 8U, ANO_angleRadToCentideg(attitude->pitch));
    ANO_putS16LE(frame, 10U, ANO_angleRadToCentideg(attitude->yaw));
    frame[12] = fusionStatus;

    for (index = 0U; index < (ANO_EULER_FRAME_LENGTH - 2U); ++index) {
        sum = (uint8_t)(sum + frame[index]);
        add = (uint8_t)(add + sum);
    }
    frame[13] = sum;
    frame[14] = add;

    for (index = 0U; index < ANO_EULER_FRAME_LENGTH; ++index) {
        DEBUG_UART_writeByte(frame[index]);
    }
}
