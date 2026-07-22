#include "imu_diag_schedule.h"

bool IMU_DIAG_isDue(uint32_t now, uint32_t last)
{
    return (uint32_t)(now - last) >= IMU_DIAG_PERIOD_TICKS;
}

uint32_t IMU_DIAG_advance(uint32_t last)
{
    return last + IMU_DIAG_PERIOD_TICKS;
}

uint32_t IMU_DIAG_reschedule(uint32_t now, uint32_t last)
{
    uint32_t next = IMU_DIAG_advance(last);

    return IMU_DIAG_isDue(now, next) ? now : next;
}
