#ifndef ICM_20948_REG_H_
#define ICM_20948_REG_H_

#define REG_VAL_SELECT_BANK_0 (0x00U)
#define REG_VAL_SELECT_BANK_1 (0x10U)
#define REG_VAL_SELECT_BANK_2 (0x20U)
#define REG_VAL_SELECT_BANK_3 (0x30U)

#define ICM20948_DEV (0x68U)
#define REG_BANK_SEL (0x7FU)

#define WHO_AM_I (0x00U)
#define USER_CTRL (0x03U)
#define PWR_MGMT_1 (0x06U)
#define PWR_MGMT_2 (0x07U)
#define INT_PIN_CFG (0x0FU)
#define ACCEL_XOUT_H (0x2DU)
#define GYRO_XOUT_H (0x33U)
#define TEMP_OUT_H (0x39U)

#define SELF_TEST_X_GYRO (0x02U)
#define SELF_TEST_Y_GYRO (0x03U)
#define SELF_TEST_Z_GYRO (0x04U)

#define GYRO_SMPLRT_DIV (0x00U)
#define GYRO_CONFIG_1 (0x01U)
#define ACCEL_SMPLRT_DIV_2 (0x11U)
#define ACCEL_CONFIG (0x14U)

#define AK09916_DEV (0x0CU)
#define WIA (0x01U)
#define ST1 (0x10U)
#define HXL (0x11U)
#define ST2 (0x18U)
#define CNTL2 (0x31U)

#endif /* ICM_20948_REG_H_ */
