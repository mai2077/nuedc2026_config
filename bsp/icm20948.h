#ifndef ICM20948_H_
#define ICM20948_H_

#include <stdint.h>

typedef enum {
    ICM20948_STATUS_OK = 0,
    ICM20948_STATUS_INVALID_ARGUMENT,
    ICM20948_STATUS_TIMEOUT,
    ICM20948_STATUS_I2C_ERROR,
    ICM20948_STATUS_WHO_AM_I_MISMATCH,
    ICM20948_STATUS_CONFIG_MISMATCH
} ICM20948_Status;

typedef struct {
    int16_t rawX;
    int16_t rawY;
    int16_t rawZ;
    int32_t milliDpsX;
    int32_t milliDpsY;
    int32_t milliDpsZ;
} ICM20948_GyroSample;

ICM20948_Status ICM20948_init(uint8_t *whoAmI);
ICM20948_Status ICM20948_selfCheck(uint8_t *whoAmI);
ICM20948_Status ICM20948_readGyro(ICM20948_GyroSample *sample);
int32_t ICM20948_rawToMilliDps(int16_t rawValue);

#endif /* ICM20948_H_ */
