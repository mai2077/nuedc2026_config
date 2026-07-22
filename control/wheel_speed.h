#ifndef WHEEL_SPEED_H_
#define WHEEL_SPEED_H_

#include <stdint.h>

#define WHEEL_SPEED_COUNTS_PER_REV             (265)
#define WHEEL_SPEED_WHEEL_DIAMETER_MM          (48)
#define WHEEL_SPEED_WHEEL_CIRCUMFERENCE_X1000_MM (150796)
#define WHEEL_SPEED_TEST_TARGET_MMPS           (400)
#define WHEEL_SPEED_MAX_TARGET_MMPS            (1500)
#define WHEEL_SPEED_PWM_LIMIT                   (700)

#define WHEEL_SPEED_RIGHT_KP_X100               (2000U)
#define WHEEL_SPEED_RIGHT_KI_X100               (0)
#define WHEEL_SPEED_LEFT_KP_X100                (2000U)
#define WHEEL_SPEED_LEFT_KI_X100                (200U)
#define WHEEL_SPEED_KP_X100_MAX                 (10000U)
#define WHEEL_SPEED_KI_X100_MAX                 (2000U)

typedef enum {
    WHEEL_SPEED_WHEEL_RIGHT = 0,
    WHEEL_SPEED_WHEEL_LEFT = 1
} WHEEL_SPEED_Wheel;

typedef struct {
    int16_t setpointMmps;
    int32_t inputMmps;
    int16_t outputPwm;
    int32_t errorMmps;
    int32_t proportionalPwm;
    int32_t integralPwm;
    int32_t derivativePwm;
    uint16_t kpX100;
    uint16_t kiX100;
} WHEEL_SPEED_Diagnostics;

void WHEEL_SPEED_init(void);
void WHEEL_SPEED_setTargetsMmps(int16_t rightMmps, int16_t leftMmps);
void WHEEL_SPEED_update10ms(
    int32_t rightCounts10ms, int32_t leftCounts10ms);
uint8_t WHEEL_SPEED_setGainsX100(
    WHEEL_SPEED_Wheel wheel, uint16_t kpX100, uint16_t kiX100);
void WHEEL_SPEED_getDiagnostics(
    WHEEL_SPEED_Wheel wheel, WHEEL_SPEED_Diagnostics *diagnostics);

int16_t WHEEL_SPEED_getRightTargetMmps(void);
int16_t WHEEL_SPEED_getLeftTargetMmps(void);
int16_t WHEEL_SPEED_getRightPwm(void);
int16_t WHEEL_SPEED_getLeftPwm(void);

int32_t WHEEL_SPEED_mmpsToCountsQ8(int32_t speedMmps);
int32_t WHEEL_SPEED_counts10msToMmps(int32_t counts10ms);

#endif /* WHEEL_SPEED_H_ */
