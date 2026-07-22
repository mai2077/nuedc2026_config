#ifndef BSP_IMU_H_
#define BSP_IMU_H_

#include <stdint.h>

#include "bsp_iic.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float x;
    float y;
    float z;
} PrivateBuf_t;

typedef struct
{
    PrivateBuf_t gyro;  /* rad/s */
    PrivateBuf_t accel; /* m/s^2 */
    PrivateBuf_t magn;  /* uT */
} IMU_DATA_t;

typedef struct
{
    float roll;
    float pitch;
    float yaw;
} ATTITUDE_DATA_t;

typedef enum
{
    IMU_DEVICE_ICM20948 = 0,
    IMU_DEVICE_MPU6500 = 1
} IMUDevice_t;

typedef enum
{
    BSP_IMU_INIT_STAGE_IDLE = 0,
    BSP_IMU_INIT_STAGE_DETECT,
    BSP_IMU_INIT_STAGE_RESET,
    BSP_IMU_INIT_STAGE_CONFIG,
    BSP_IMU_INIT_STAGE_MAG,
    BSP_IMU_INIT_STAGE_READY
} BSP_IMU_INIT_STAGE_t;

typedef struct
{
    IMUDevice_t selected_device;
    BSP_IMU_INIT_STAGE_t init_stage;
    uint8_t last_addr_7bit;
    uint8_t last_who_am_i;
    uint8_t last_mag_wia;
    uint8_t mag_available;
    uint8_t init_ok;
    IIC_Status_t last_iic_status;
} bsp_imu_diag_t;

#ifndef BSP_IMU_DEFAULT_DEVICE
#define BSP_IMU_DEFAULT_DEVICE IMU_DEVICE_ICM20948
#endif

typedef struct
{
    uint8_t (*Init)(void);
    uint8_t (*DeInit)(void);
    void (*UpdateZeroPoint_axis)(const IMU_DATA_t *point);
    void (*UpdateZeroPoint_attitude)(const ATTITUDE_DATA_t *attitude);
    void (*Update_9axisVal)(IMU_DATA_t *imudata);
    void (*UpdateAttitude)(IMU_DATA_t imudata,
                           ATTITUDE_DATA_t *attitude);
} IMUInterface_t;

typedef IMUInterface_t *pIMUInterface_t;

void bsp_imu_select_device(IMUDevice_t device);
IMUDevice_t bsp_imu_get_selected_device(void);
const char *bsp_imu_get_device_name(IMUDevice_t device);
pIMUInterface_t bsp_imu_get_interface(void);
void bsp_imu_get_diag(bsp_imu_diag_t *p_diag);
uint8_t bsp_imu_update_9axis_checked(IMU_DATA_t *imudata);
uint32_t bsp_imu_port_get_tick_ms(void);

extern IMUInterface_t UserICM20948;
extern IMUInterface_t UserMPU9250;

#ifdef __cplusplus
}
#endif

#endif /* BSP_IMU_H_ */
