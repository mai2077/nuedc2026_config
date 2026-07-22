#ifdef PUBLIC_API_HOST_TEST

#include "bsp_iic.h"
#include "bsp_imu.h"

static void compile_public_api(void)
{
    IICInterface_t iic = {0};
    IMU_DATA_t motion = {0};
    ATTITUDE_DATA_t attitude = {0};
    IMUInterface_t imu = {0};

    (void)iic;
    (void)motion;
    (void)attitude;
    (void)imu;
    (void)&UserII2Dev;
    (void)&UserICM20948;
    (void)&UserMPU9250;
    (void)&bsp_imu_update_9axis_checked;
    (void)&bsp_imu_port_get_tick_ms;
}

int main(void)
{
    compile_public_api();
    return 0;
}

#endif /* PUBLIC_API_HOST_TEST */
