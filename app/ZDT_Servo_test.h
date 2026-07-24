#ifndef ZDT_SERVO_TEST_H_
#define ZDT_SERVO_TEST_H_

#include <stdint.h>

#include "bsp/bsp_ZDT_Servo_emm_v5.h"

#ifndef ZDT_SERVO_STANDALONE_TEST
#define ZDT_SERVO_STANDALONE_TEST       (1U)
#endif

#define ZDT_SERVO_TEST_ADDRESS          (0x01U)
#define ZDT_SERVO_TEST_SPEED_RPM        (60U)
#define ZDT_SERVO_TEST_ACCEL            (10U)
#define ZDT_SERVO_PULSES_PER_REV        (3200L)
#define ZDT_SERVO_ANGLE_LIMIT_DEG       (360)

void ZDT_Servo_Test_init(void);
void ZDT_Servo_Test_update10ms(void);
BSP_ZDT_Servo_Status ZDT_Servo_rotateDegrees(int16_t angleDegrees);

#endif /* ZDT_SERVO_TEST_H_ */
