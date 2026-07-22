#ifdef IMU_DIAG_SCHEDULE_HOST_TEST

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "imu_diag_schedule.h"

static void testTwoTickPeriod(void)
{
    assert(!IMU_DIAG_isDue(1U, 0U));
    assert(IMU_DIAG_isDue(2U, 0U));
    assert(IMU_DIAG_advance(0U) == 2U);
}

static void testTickWraparound(void)
{
    uint32_t last = UINT32_MAX;

    assert(!IMU_DIAG_isDue(0U, last));
    assert(IMU_DIAG_isDue(1U, last));
    assert(IMU_DIAG_advance(last) == 1U);
}

static void testLateExecutionDropsBacklog(void)
{
    assert(IMU_DIAG_reschedule(2U, 0U) == 2U);
    assert(IMU_DIAG_reschedule(15U, 0U) == 15U);
}

int main(void)
{
    testTwoTickPeriod();
    testTickWraparound();
    testLateExecutionDropsBacklog();
    puts("scheduler tests passed");
    return 0;
}

#endif /* IMU_DIAG_SCHEDULE_HOST_TEST */
