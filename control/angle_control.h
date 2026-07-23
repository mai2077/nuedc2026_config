#ifndef ANGLE_CONTROL_H_
#define ANGLE_CONTROL_H_

#include <stdint.h>

#define ANGLE_CONTROL_TURN_SPEED_MMPS     (220)
#define ANGLE_CONTROL_TURN_TOLERANCE_DEG  (5.0f)
#define ANGLE_CONTROL_TURN_HOLD_TICKS     (50U)
#define ANGLE_CONTROL_FORWARD_FIXED_TICKS (50U)
#define ANGLE_CONTROL_FORWARD_CORRECTION_LIMIT_MMPS (200)

typedef enum {
    ANGLE_CONTROL_STATE_IDLE = 0,
    ANGLE_CONTROL_STATE_DRIVE_PID,
    ANGLE_CONTROL_STATE_TURN_OPEN_LOOP,
    ANGLE_CONTROL_STATE_TURN_HOLD,
    ANGLE_CONTROL_STATE_FORWARD_FIXED
} ANGLE_CONTROL_State;

typedef struct {
    int16_t rightTargetMmps;
    int16_t leftTargetMmps;
    ANGLE_CONTROL_State state;
    float currentYawDeg;
    float targetYawDeg;
    float errorDeg;
    float turnMmps;
} ANGLE_CONTROL_Output;

void ANGLE_CONTROL_init(void);
void ANGLE_CONTROL_disable(void);
uint8_t ANGLE_CONTROL_isActive(void);
void ANGLE_CONTROL_setBaseSpeedMmps(int16_t baseSpeedMmps);
void ANGLE_CONTROL_setPid(float kp, float ki, float kd);
void ANGLE_CONTROL_setTargetYawDeg(float targetYawDeg);
float ANGLE_CONTROL_wrapErrorDeg(float targetYawDeg, float currentYawDeg);
void ANGLE_CONTROL_update10ms(
    float currentYawDeg, ANGLE_CONTROL_Output *output);
void ANGLE_CONTROL_getOutput(ANGLE_CONTROL_Output *output);
const char *ANGLE_CONTROL_stateName(ANGLE_CONTROL_State state);

#endif /* ANGLE_CONTROL_H_ */
