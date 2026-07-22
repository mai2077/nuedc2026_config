#include "bsp_imu.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#include "icm_20948_reg.h"
#include "mpu9250_reg.h"

#if defined(__GNUC__) || defined(__clang__)
#define BSP_IMU_WEAK __attribute__((weak))
#else
#define BSP_IMU_WEAK
#endif

#define IMU_GYRO_LPF_ALPHA (0.80f)
#define IMU_ACCEL_LPF_ALPHA (0.70f)
#define IMU_MAG_LPF_ALPHA (0.85f)
#define IMU_EINT_LIMIT (2.0f)
#define IMU_MIN_DT_S (0.001f)
#define IMU_MAX_DT_S (0.050f)
#define IMU_DEFAULT_DT_S (0.020f)
#define IMU_ACCEL_SCALE_2G (0.00059814453125f)
#define IMU_GYRO_SCALE_2000DPS (0.06103515625f)
#define IMU_DEG_TO_RAD (0.01745329251994329577f)
#define IMU_MAG_SCALE_UT (0.1495361328125f)

#define ICM_ADDRESS_8BIT (ICM20948_DEV << 1U)
#define ICM_WHO_AM_I_VALUE (0xEAU)
#define ICM_GYRO_DIV_VALUE (0x04U)
#define ICM_GYRO_CONFIG_VALUE (0x1FU)
#define ICM_ACCEL_DIV_VALUE (0x04U)
#define ICM_ACCEL_CONFIG_VALUE (0x29U)
#define ICM_BYPASS_VALUE (0x02U)
#define AK09916_WHO_AM_I_VALUE (0x09U)
#define AK09916_CONTINUOUS_100HZ (0x08U)

typedef struct
{
    IMU_DATA_t zero_point;
    ATTITUDE_DATA_t zero_attitude;
    IMU_DATA_t lpf_state;
    uint8_t lpf_ready;
    float e_int[3];
    float q[4];
    uint32_t last_tick_ms;
    uint8_t attitude_ready;
} imu_runtime_state_t;

static imu_runtime_state_t g_icm_state;
static imu_runtime_state_t g_mpu_state;
static IMUDevice_t g_selected_device = BSP_IMU_DEFAULT_DEVICE;
static BSP_IMU_INIT_STAGE_t g_icm_init_stage = BSP_IMU_INIT_STAGE_IDLE;
static BSP_IMU_INIT_STAGE_t g_mpu_init_stage = BSP_IMU_INIT_STAGE_IDLE;
static uint8_t g_icm_who_am_i;
static uint8_t g_icm_mag_wia;
static uint8_t g_icm_mag_available;
static uint8_t g_mpu_address_7bit = MPU9250_DEV_ADDR;
static uint8_t g_mpu_who_am_i;
static uint8_t g_mpu_mag_wia;
static uint8_t g_mpu_mag_available;
static IIC_Status_t g_last_iic_status = IIC_OK;

static float imu_clampf(float value, float minimum, float maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static float imu_lpf(float previous, float current, float alpha)
{
    return alpha * previous + (1.0f - alpha) * current;
}

static int16_t imu_read_s16_be(const uint8_t *data)
{
    return (int16_t)(((uint16_t)data[0] << 8U) | data[1]);
}

static int16_t imu_read_s16_le(const uint8_t *data)
{
    return (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8U));
}

static uint8_t imu_write_register(uint16_t device,
                                  uint16_t reg,
                                  uint8_t value,
                                  unsigned long timeout)
{
    g_last_iic_status =
        UserII2Dev.write_reg(device, reg, &value, 1U, timeout);
    return (uint8_t)(g_last_iic_status != IIC_OK);
}

static uint8_t imu_read_registers(uint16_t device,
                                  uint16_t reg,
                                  uint8_t *data,
                                  uint16_t size,
                                  unsigned long timeout)
{
    g_last_iic_status =
        UserII2Dev.read_reg(device, reg, data, size, timeout);
    return (uint8_t)(g_last_iic_status != IIC_OK);
}

static uint8_t icm_select_bank(uint8_t bank)
{
    return imu_write_register(ICM_ADDRESS_8BIT, REG_BANK_SEL, bank, 100U);
}

static void imu_runtime_reset(imu_runtime_state_t *state)
{
    if (state == NULL) {
        return;
    }
    memset(state, 0, sizeof(*state));
    state->q[0] = 1.0f;
}

static void imu_apply_lpf(IMU_DATA_t *data, imu_runtime_state_t *state)
{
    if ((data == NULL) || (state == NULL)) {
        return;
    }

    if (state->lpf_ready == 0U) {
        state->lpf_state = *data;
        state->lpf_ready = 1U;
        return;
    }

    state->lpf_state.accel.x =
        imu_lpf(state->lpf_state.accel.x, data->accel.x,
                IMU_ACCEL_LPF_ALPHA);
    state->lpf_state.accel.y =
        imu_lpf(state->lpf_state.accel.y, data->accel.y,
                IMU_ACCEL_LPF_ALPHA);
    state->lpf_state.accel.z =
        imu_lpf(state->lpf_state.accel.z, data->accel.z,
                IMU_ACCEL_LPF_ALPHA);
    state->lpf_state.gyro.x =
        imu_lpf(state->lpf_state.gyro.x, data->gyro.x,
                IMU_GYRO_LPF_ALPHA);
    state->lpf_state.gyro.y =
        imu_lpf(state->lpf_state.gyro.y, data->gyro.y,
                IMU_GYRO_LPF_ALPHA);
    state->lpf_state.gyro.z =
        imu_lpf(state->lpf_state.gyro.z, data->gyro.z,
                IMU_GYRO_LPF_ALPHA);
    state->lpf_state.magn.x =
        imu_lpf(state->lpf_state.magn.x, data->magn.x,
                IMU_MAG_LPF_ALPHA);
    state->lpf_state.magn.y =
        imu_lpf(state->lpf_state.magn.y, data->magn.y,
                IMU_MAG_LPF_ALPHA);
    state->lpf_state.magn.z =
        imu_lpf(state->lpf_state.magn.z, data->magn.z,
                IMU_MAG_LPF_ALPHA);
    *data = state->lpf_state;
}

static void imu_update_zero_axis(imu_runtime_state_t *state,
                                 const IMU_DATA_t *point)
{
    if ((state == NULL) || (point == NULL)) {
        return;
    }
    state->zero_point = *point;
    state->lpf_ready = 0U;
}

static void imu_update_zero_attitude(imu_runtime_state_t *state,
                                     const ATTITUDE_DATA_t *attitude)
{
    if ((state == NULL) || (attitude == NULL)) {
        return;
    }
    state->zero_attitude = *attitude;
}

static float imu_delta_time_s(imu_runtime_state_t *state)
{
    uint32_t now_ms;
    uint32_t delta_ms;

    if (state == NULL) {
        return IMU_DEFAULT_DT_S;
    }

    now_ms = bsp_imu_port_get_tick_ms();
    if (now_ms == 0U) {
        return IMU_DEFAULT_DT_S;
    }
    if (state->last_tick_ms == 0U) {
        state->last_tick_ms = now_ms;
        return IMU_DEFAULT_DT_S;
    }

    delta_ms = now_ms - state->last_tick_ms;
    state->last_tick_ms = now_ms;
    if (delta_ms == 0U) {
        return IMU_MIN_DT_S;
    }
    if (delta_ms > 50U) {
        return IMU_MAX_DT_S;
    }
    return (float)delta_ms * 0.001f;
}

static void imu_init_attitude_from_accel(imu_runtime_state_t *state,
                                         const IMU_DATA_t *data)
{
    float roll;
    float pitch;
    float norm;
    float cr;
    float sr;
    float cp;
    float sp;

    norm = sqrtf(data->accel.x * data->accel.x +
                 data->accel.y * data->accel.y +
                 data->accel.z * data->accel.z);
    if (norm < 0.001f) {
        return;
    }

    roll = atan2f(data->accel.y, data->accel.z);
    pitch = atan2f(-data->accel.x,
                   sqrtf(data->accel.y * data->accel.y +
                         data->accel.z * data->accel.z));
    cr = cosf(roll * 0.5f);
    sr = sinf(roll * 0.5f);
    cp = cosf(pitch * 0.5f);
    sp = sinf(pitch * 0.5f);
    state->q[0] = cr * cp;
    state->q[1] = sr * cp;
    state->q[2] = cr * sp;
    state->q[3] = -sr * sp;
    state->e_int[0] = 0.0f;
    state->e_int[1] = 0.0f;
    state->e_int[2] = 0.0f;
    state->last_tick_ms = 0U;
    state->attitude_ready = 1U;
}

static void imu_update_attitude(imu_runtime_state_t *state,
                                IMU_DATA_t data,
                                ATTITUDE_DATA_t *attitude)
{
    const float kp = 0.8f;
    const float ki = 0.0001f;
    float dt;
    float half_dt;
    float q0;
    float q1;
    float q2;
    float q3;
    float vx;
    float vy;
    float vz;
    float ax;
    float ay;
    float az;
    float gx;
    float gy;
    float gz;
    float ex;
    float ey;
    float norm;
    float nq0;
    float nq1;
    float nq2;
    float nq3;

    if ((state == NULL) || (attitude == NULL)) {
        return;
    }
    if (state->attitude_ready == 0U) {
        imu_init_attitude_from_accel(state, &data);
    }

    ax = data.accel.x;
    ay = data.accel.y;
    az = data.accel.z;
    norm = sqrtf(ax * ax + ay * ay + az * az);
    if (norm < 0.000001f) {
        return;
    }
    ax /= norm;
    ay /= norm;
    az /= norm;

    q0 = state->q[0];
    q1 = state->q[1];
    q2 = state->q[2];
    q3 = state->q[3];
    vx = 2.0f * (q1 * q3 - q0 * q2);
    vy = 2.0f * (q0 * q1 + q2 * q3);
    vz = 1.0f - 2.0f * (q1 * q1 + q2 * q2);
    ex = ay * vz - az * vy;
    ey = az * vx - ax * vz;
    state->e_int[0] = imu_clampf(state->e_int[0] + ex,
                                 -IMU_EINT_LIMIT, IMU_EINT_LIMIT);
    state->e_int[1] = imu_clampf(state->e_int[1] + ey,
                                 -IMU_EINT_LIMIT, IMU_EINT_LIMIT);

    gx = data.gyro.x + kp * ex + ki * state->e_int[0];
    gy = data.gyro.y + kp * ey + ki * state->e_int[1];
    gz = data.gyro.z;
    dt = imu_clampf(imu_delta_time_s(state), IMU_MIN_DT_S, IMU_MAX_DT_S);
    half_dt = dt * 0.5f;
    nq0 = q0 + (-q1 * gx - q2 * gy - q3 * gz) * half_dt;
    nq1 = q1 + (q0 * gx + q2 * gz - q3 * gy) * half_dt;
    nq2 = q2 + (q0 * gy - q1 * gz + q3 * gx) * half_dt;
    nq3 = q3 + (q0 * gz + q1 * gy - q2 * gx) * half_dt;
    norm = sqrtf(nq0 * nq0 + nq1 * nq1 + nq2 * nq2 + nq3 * nq3);
    if (norm < 0.000001f) {
        return;
    }
    state->q[0] = nq0 / norm;
    state->q[1] = nq1 / norm;
    state->q[2] = nq2 / norm;
    state->q[3] = nq3 / norm;

    q0 = state->q[0];
    q1 = state->q[1];
    q2 = state->q[2];
    q3 = state->q[3];
    attitude->roll = atan2f(2.0f * (q2 * q3 + q0 * q1),
                            1.0f - 2.0f * (q1 * q1 + q2 * q2));
    attitude->pitch = asinf(imu_clampf(2.0f * (q0 * q2 - q1 * q3),
                                      -1.0f, 1.0f));
    attitude->yaw = atan2f(2.0f * (q1 * q2 + q0 * q3),
                           q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3);
    attitude->roll -= state->zero_attitude.roll;
    attitude->pitch -= state->zero_attitude.pitch;
    attitude->yaw -= state->zero_attitude.yaw;
}

static uint8_t icm_init(void)
{
    uint8_t who = 0U;
    uint8_t mag_wia = 0U;
    uint8_t retry;

    g_icm_init_stage = BSP_IMU_INIT_STAGE_DETECT;
    g_icm_who_am_i = 0U;
    g_icm_mag_wia = 0U;
    g_icm_mag_available = 0U;
    g_last_iic_status = IIC_OK;

    if ((icm_select_bank(REG_VAL_SELECT_BANK_0) != 0U) ||
        (imu_read_registers(ICM_ADDRESS_8BIT, WHO_AM_I, &who, 1U,
                            500U) != 0U)) {
        return 1U;
    }
    g_icm_who_am_i = who;
    if (who != ICM_WHO_AM_I_VALUE) {
        g_last_iic_status = IIC_ERR;
        return 1U;
    }

    g_icm_init_stage = BSP_IMU_INIT_STAGE_RESET;
    if (imu_write_register(ICM_ADDRESS_8BIT, PWR_MGMT_1, 0x80U,
                           500U) != 0U) {
        return 1U;
    }
    UserII2Dev.delay_ms(100U);
    if ((imu_write_register(ICM_ADDRESS_8BIT, USER_CTRL, 0x00U,
                            100U) != 0U) ||
        (imu_write_register(ICM_ADDRESS_8BIT, PWR_MGMT_1, 0x01U,
                            100U) != 0U) ||
        (imu_write_register(ICM_ADDRESS_8BIT, PWR_MGMT_2, 0x00U,
                            100U) != 0U)) {
        return 1U;
    }
    UserII2Dev.delay_ms(35U);

    g_icm_init_stage = BSP_IMU_INIT_STAGE_CONFIG;
    if ((icm_select_bank(REG_VAL_SELECT_BANK_2) != 0U) ||
        (imu_write_register(ICM_ADDRESS_8BIT, GYRO_SMPLRT_DIV,
                            ICM_GYRO_DIV_VALUE, 100U) != 0U) ||
        (imu_write_register(ICM_ADDRESS_8BIT, GYRO_CONFIG_1,
                            ICM_GYRO_CONFIG_VALUE, 100U) != 0U) ||
        (imu_write_register(ICM_ADDRESS_8BIT, ACCEL_SMPLRT_DIV_2,
                            ICM_ACCEL_DIV_VALUE, 100U) != 0U) ||
        (imu_write_register(ICM_ADDRESS_8BIT, ACCEL_CONFIG,
                            ICM_ACCEL_CONFIG_VALUE, 100U) != 0U) ||
        (icm_select_bank(REG_VAL_SELECT_BANK_0) != 0U) ||
        (imu_write_register(ICM_ADDRESS_8BIT, INT_PIN_CFG,
                            ICM_BYPASS_VALUE, 100U) != 0U)) {
        return 1U;
    }

    g_icm_init_stage = BSP_IMU_INIT_STAGE_MAG;
    for (retry = 0U; retry < 5U; ++retry) {
        if (imu_read_registers((uint16_t)(AK09916_DEV << 1U), WIA,
                               &mag_wia, 1U, 100U) == 0U) {
            g_icm_mag_wia = mag_wia;
            if (mag_wia == AK09916_WHO_AM_I_VALUE) {
                break;
            }
        }
        UserII2Dev.delay_ms(10U);
    }
    if (mag_wia != AK09916_WHO_AM_I_VALUE) {
        g_last_iic_status = IIC_ERR;
        return 1U;
    }
    if (imu_write_register((uint16_t)(AK09916_DEV << 1U), CNTL2,
                           AK09916_CONTINUOUS_100HZ, 100U) != 0U) {
        return 1U;
    }

    g_icm_mag_available = 1U;
    imu_runtime_reset(&g_icm_state);
    g_icm_init_stage = BSP_IMU_INIT_STAGE_READY;
    return 0U;
}

static uint8_t icm_deinit(void)
{
    uint8_t who = 0U;

    if ((icm_select_bank(REG_VAL_SELECT_BANK_0) != 0U) ||
        (imu_read_registers(ICM_ADDRESS_8BIT, WHO_AM_I, &who, 1U,
                            500U) != 0U) ||
        (who != ICM_WHO_AM_I_VALUE) ||
        (imu_write_register(ICM_ADDRESS_8BIT, PWR_MGMT_1, 0x40U,
                            500U) != 0U)) {
        return 1U;
    }
    imu_runtime_reset(&g_icm_state);
    g_icm_init_stage = BSP_IMU_INIT_STAGE_IDLE;
    g_icm_mag_available = 0U;
    return 0U;
}

static uint8_t icm_read_motion(IMU_DATA_t *output)
{
    uint8_t motion[12];
    uint8_t magnetic[8];
    IMU_DATA_t sample;

    if (output == NULL) {
        g_last_iic_status = IIC_ERR;
        return 1U;
    }
    if ((icm_select_bank(REG_VAL_SELECT_BANK_0) != 0U) ||
        (imu_read_registers(ICM_ADDRESS_8BIT, ACCEL_XOUT_H, motion,
                            sizeof(motion), 100U) != 0U)) {
        return 1U;
    }

    memset(&sample, 0, sizeof(sample));
    sample.accel.x = (float)imu_read_s16_be(&motion[0]) *
                     IMU_ACCEL_SCALE_2G;
    sample.accel.y = (float)imu_read_s16_be(&motion[2]) *
                     IMU_ACCEL_SCALE_2G;
    sample.accel.z = (float)imu_read_s16_be(&motion[4]) *
                     IMU_ACCEL_SCALE_2G;
    sample.gyro.x = (float)imu_read_s16_be(&motion[6]) *
                    IMU_GYRO_SCALE_2000DPS * IMU_DEG_TO_RAD;
    sample.gyro.y = (float)imu_read_s16_be(&motion[8]) *
                    IMU_GYRO_SCALE_2000DPS * IMU_DEG_TO_RAD;
    sample.gyro.z = (float)imu_read_s16_be(&motion[10]) *
                    IMU_GYRO_SCALE_2000DPS * IMU_DEG_TO_RAD;

    if (g_icm_mag_available != 0U) {
        if (imu_read_registers((uint16_t)(AK09916_DEV << 1U), HXL,
                               magnetic, sizeof(magnetic), 100U) != 0U) {
            /* ID 0x01 still has a valid accel/gyro sample without AK09916. */
            g_icm_mag_available = 0U;
        } else if ((magnetic[7] & 0x08U) == 0U) {
            sample.magn.x = (float)imu_read_s16_le(&magnetic[0]) *
                            IMU_MAG_SCALE_UT;
            sample.magn.y = (float)imu_read_s16_le(&magnetic[2]) *
                            IMU_MAG_SCALE_UT;
            sample.magn.z = (float)imu_read_s16_le(&magnetic[4]) *
                            IMU_MAG_SCALE_UT;
        }
    }

    sample.accel.x -= g_icm_state.zero_point.accel.x;
    sample.accel.y -= g_icm_state.zero_point.accel.y;
    sample.accel.z -= g_icm_state.zero_point.accel.z;
    sample.gyro.x -= g_icm_state.zero_point.gyro.x;
    sample.gyro.y -= g_icm_state.zero_point.gyro.y;
    sample.gyro.z -= g_icm_state.zero_point.gyro.z;
    sample.magn.x -= g_icm_state.zero_point.magn.x;
    sample.magn.y -= g_icm_state.zero_point.magn.y;
    sample.magn.z -= g_icm_state.zero_point.magn.z;
    imu_apply_lpf(&sample, &g_icm_state);
    *output = sample;
    return 0U;
}

static void icm_update_zero_axis(const IMU_DATA_t *point)
{
    imu_update_zero_axis(&g_icm_state, point);
}

static void icm_update_zero_attitude(const ATTITUDE_DATA_t *attitude)
{
    imu_update_zero_attitude(&g_icm_state, attitude);
}

static void icm_update_motion(IMU_DATA_t *data)
{
    (void)icm_read_motion(data);
}

static void icm_update_attitude(IMU_DATA_t data,
                                ATTITUDE_DATA_t *attitude)
{
    imu_update_attitude(&g_icm_state, data, attitude);
}

static uint8_t mpu_supported_id(uint8_t who)
{
    return (uint8_t)((who == MPU9250_WHO_AM_I_VAL) ||
                     (who == MPU6500_WHO_AM_I_VAL) ||
                     (who == MPU9255_WHO_AM_I_VAL));
}

static uint8_t mpu_detect(void)
{
    static const uint8_t candidates[] = {
        MPU9250_DEV_ADDR, MPU9250_DEV_ADDR_ALT};
    uint8_t index;

    for (index = 0U; index < sizeof(candidates); ++index) {
        uint8_t who = 0U;
        uint16_t address = (uint16_t)candidates[index] << 1U;

        if ((imu_read_registers(address, MPU9250_WHO_AM_I, &who, 1U,
                                100U) == 0U) &&
            (mpu_supported_id(who) != 0U)) {
            g_mpu_address_7bit = candidates[index];
            g_mpu_who_am_i = who;
            return 0U;
        }
    }
    return 1U;
}

static uint8_t mpu_write(uint16_t reg, uint8_t value)
{
    return imu_write_register((uint16_t)g_mpu_address_7bit << 1U,
                              reg, value, 100U);
}

static uint8_t mpu_init(void)
{
    uint8_t retry;
    uint8_t mag_wia = 0U;

    g_mpu_init_stage = BSP_IMU_INIT_STAGE_DETECT;
    g_mpu_mag_available = 0U;
    g_mpu_mag_wia = 0U;
    g_mpu_who_am_i = 0U;
    if (mpu_detect() != 0U) {
        return 1U;
    }

    g_mpu_init_stage = BSP_IMU_INIT_STAGE_RESET;
    if (mpu_write(MPU9250_PWR_MGMT_1, 0x80U) != 0U) {
        return 1U;
    }
    UserII2Dev.delay_ms(100U);
    if ((mpu_write(MPU9250_PWR_MGMT_2, 0x00U) != 0U) ||
        (mpu_write(MPU9250_PWR_MGMT_1, 0x01U) != 0U) ||
        (mpu_write(MPU9250_USER_CTRL, 0x00U) != 0U)) {
        return 1U;
    }

    g_mpu_init_stage = BSP_IMU_INIT_STAGE_CONFIG;
    if ((mpu_write(MPU9250_CONFIG, 0x03U) != 0U) ||
        (mpu_write(MPU9250_SMPLRT_DIV, 0x04U) != 0U) ||
        (mpu_write(MPU9250_GYRO_CONFIG, 0x18U) != 0U) ||
        (mpu_write(MPU9250_ACCEL_CONFIG, 0x00U) != 0U) ||
        (mpu_write(MPU9250_ACCEL_CONFIG_2, 0x03U) != 0U) ||
        (mpu_write(MPU9250_INT_PIN_CFG, 0x02U) != 0U)) {
        return 1U;
    }

    g_mpu_init_stage = BSP_IMU_INIT_STAGE_MAG;
    for (retry = 0U; retry < 10U; ++retry) {
        if ((imu_read_registers((uint16_t)(AK8963_DEV_ADDR << 1U),
                                AK8963_WIA, &mag_wia, 1U, 100U) == 0U) &&
            (mag_wia == AK8963_WIA_VAL)) {
            break;
        }
        UserII2Dev.delay_ms(10U);
    }
    g_mpu_mag_wia = mag_wia;
    if (mag_wia == AK8963_WIA_VAL) {
        if ((imu_write_register((uint16_t)(AK8963_DEV_ADDR << 1U),
                                AK8963_CNTL2, 0x01U, 100U) != 0U) ||
            (imu_write_register((uint16_t)(AK8963_DEV_ADDR << 1U),
                                AK8963_CNTL1, 0x16U, 100U) != 0U)) {
            return 1U;
        }
        g_mpu_mag_available = 1U;
    }

    imu_runtime_reset(&g_mpu_state);
    g_mpu_init_stage = BSP_IMU_INIT_STAGE_READY;
    return 0U;
}

static uint8_t mpu_deinit(void)
{
    if (mpu_write(MPU9250_PWR_MGMT_1, 0x40U) != 0U) {
        return 1U;
    }
    imu_runtime_reset(&g_mpu_state);
    g_mpu_init_stage = BSP_IMU_INIT_STAGE_IDLE;
    g_mpu_mag_available = 0U;
    return 0U;
}

static uint8_t mpu_read_motion(IMU_DATA_t *output)
{
    uint8_t motion[14];
    uint8_t st1 = 0U;
    uint8_t magnetic[7];
    IMU_DATA_t sample;

    if (output == NULL) {
        return 1U;
    }
    if (imu_read_registers((uint16_t)g_mpu_address_7bit << 1U,
                           MPU9250_ACCEL_XOUT_H, motion, sizeof(motion),
                           100U) != 0U) {
        return 1U;
    }

    memset(&sample, 0, sizeof(sample));
    sample.accel.x = (float)imu_read_s16_be(&motion[0]) *
                     IMU_ACCEL_SCALE_2G;
    sample.accel.y = (float)imu_read_s16_be(&motion[2]) *
                     IMU_ACCEL_SCALE_2G;
    sample.accel.z = (float)imu_read_s16_be(&motion[4]) *
                     IMU_ACCEL_SCALE_2G;
    sample.gyro.x = (float)imu_read_s16_be(&motion[8]) *
                    IMU_GYRO_SCALE_2000DPS * IMU_DEG_TO_RAD;
    sample.gyro.y = (float)imu_read_s16_be(&motion[10]) *
                    IMU_GYRO_SCALE_2000DPS * IMU_DEG_TO_RAD;
    sample.gyro.z = (float)imu_read_s16_be(&motion[12]) *
                    IMU_GYRO_SCALE_2000DPS * IMU_DEG_TO_RAD;

    if (g_mpu_mag_available != 0U) {
        if ((imu_read_registers((uint16_t)(AK8963_DEV_ADDR << 1U),
                                AK8963_ST1, &st1, 1U, 100U) != 0U) ||
            (((st1 & 0x01U) != 0U) &&
             (imu_read_registers((uint16_t)(AK8963_DEV_ADDR << 1U),
                                 AK8963_HXL, magnetic,
                                 sizeof(magnetic), 100U) != 0U))) {
            return 1U;
        }
        if (((st1 & 0x01U) != 0U) && ((magnetic[6] & 0x08U) == 0U)) {
            sample.magn.x = (float)imu_read_s16_le(&magnetic[0]) *
                            IMU_MAG_SCALE_UT;
            sample.magn.y = (float)imu_read_s16_le(&magnetic[2]) *
                            IMU_MAG_SCALE_UT;
            sample.magn.z = (float)imu_read_s16_le(&magnetic[4]) *
                            IMU_MAG_SCALE_UT;
        }
    }

    sample.accel.x -= g_mpu_state.zero_point.accel.x;
    sample.accel.y -= g_mpu_state.zero_point.accel.y;
    sample.accel.z -= g_mpu_state.zero_point.accel.z;
    sample.gyro.x -= g_mpu_state.zero_point.gyro.x;
    sample.gyro.y -= g_mpu_state.zero_point.gyro.y;
    sample.gyro.z -= g_mpu_state.zero_point.gyro.z;
    sample.magn.x -= g_mpu_state.zero_point.magn.x;
    sample.magn.y -= g_mpu_state.zero_point.magn.y;
    sample.magn.z -= g_mpu_state.zero_point.magn.z;
    imu_apply_lpf(&sample, &g_mpu_state);
    *output = sample;
    return 0U;
}

static void mpu_update_zero_axis(const IMU_DATA_t *point)
{
    imu_update_zero_axis(&g_mpu_state, point);
}

static void mpu_update_zero_attitude(const ATTITUDE_DATA_t *attitude)
{
    imu_update_zero_attitude(&g_mpu_state, attitude);
}

static void mpu_update_motion(IMU_DATA_t *data)
{
    (void)mpu_read_motion(data);
}

static void mpu_update_attitude(IMU_DATA_t data,
                                ATTITUDE_DATA_t *attitude)
{
    imu_update_attitude(&g_mpu_state, data, attitude);
}

void bsp_imu_select_device(IMUDevice_t device)
{
    if ((device == IMU_DEVICE_ICM20948) ||
        (device == IMU_DEVICE_MPU6500)) {
        g_selected_device = device;
    }
}

IMUDevice_t bsp_imu_get_selected_device(void)
{
    return g_selected_device;
}

const char *bsp_imu_get_device_name(IMUDevice_t device)
{
    switch (device) {
        case IMU_DEVICE_ICM20948:
            return "ICM20948";
        case IMU_DEVICE_MPU6500:
            return "MPU6500";
        default:
            return "UNKNOWN";
    }
}

pIMUInterface_t bsp_imu_get_interface(void)
{
    return (g_selected_device == IMU_DEVICE_MPU6500) ?
        &UserMPU9250 : &UserICM20948;
}

void bsp_imu_get_diag(bsp_imu_diag_t *diag)
{
    if (diag == NULL) {
        return;
    }
    memset(diag, 0, sizeof(*diag));
    diag->selected_device = g_selected_device;
    diag->last_iic_status = g_last_iic_status;

    if (g_selected_device == IMU_DEVICE_MPU6500) {
        diag->init_stage = g_mpu_init_stage;
        diag->last_addr_7bit = g_mpu_address_7bit;
        diag->last_who_am_i = g_mpu_who_am_i;
        diag->last_mag_wia = g_mpu_mag_wia;
        diag->mag_available = g_mpu_mag_available;
        diag->init_ok = (uint8_t)(g_mpu_init_stage ==
                                  BSP_IMU_INIT_STAGE_READY);
    } else {
        diag->init_stage = g_icm_init_stage;
        diag->last_addr_7bit = ICM20948_DEV;
        diag->last_who_am_i = g_icm_who_am_i;
        diag->last_mag_wia = g_icm_mag_wia;
        diag->mag_available = g_icm_mag_available;
        diag->init_ok = (uint8_t)(g_icm_init_stage ==
                                  BSP_IMU_INIT_STAGE_READY);
    }
}

uint8_t bsp_imu_update_9axis_checked(IMU_DATA_t *data)
{
    return (g_selected_device == IMU_DEVICE_MPU6500) ?
        mpu_read_motion(data) : icm_read_motion(data);
}

BSP_IMU_WEAK uint32_t bsp_imu_port_get_tick_ms(void)
{
    return 0U;
}

IMUInterface_t UserICM20948 = {
    .Init = icm_init,
    .DeInit = icm_deinit,
    .UpdateZeroPoint_axis = icm_update_zero_axis,
    .UpdateZeroPoint_attitude = icm_update_zero_attitude,
    .Update_9axisVal = icm_update_motion,
    .UpdateAttitude = icm_update_attitude
};

IMUInterface_t UserMPU9250 = {
    .Init = mpu_init,
    .DeInit = mpu_deinit,
    .UpdateZeroPoint_axis = mpu_update_zero_axis,
    .UpdateZeroPoint_attitude = mpu_update_zero_attitude,
    .Update_9axisVal = mpu_update_motion,
    .UpdateAttitude = mpu_update_attitude
};
