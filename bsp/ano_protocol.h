#ifndef ANO_PROTOCOL_H_
#define ANO_PROTOCOL_H_

#include <stdint.h>

#include "bsp_imu.h"

#ifdef __cplusplus
extern "C" {
#endif

void ANO_sendImuFrame(const IMU_DATA_t *sample);
int16_t ANO_accelToCmss(float value);
int16_t ANO_gyroToCounts(float value);

#ifdef __cplusplus
}
#endif

#endif /* ANO_PROTOCOL_H_ */
