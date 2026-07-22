#ifndef IMU_DIAG_SCHEDULE_H_
#define IMU_DIAG_SCHEDULE_H_

#include <stdbool.h>
#include <stdint.h>

#define IMU_DIAG_PERIOD_TICKS (2U)

bool IMU_DIAG_isDue(uint32_t now, uint32_t last);
uint32_t IMU_DIAG_advance(uint32_t last);
uint32_t IMU_DIAG_reschedule(uint32_t now, uint32_t last);

#endif /* IMU_DIAG_SCHEDULE_H_ */
