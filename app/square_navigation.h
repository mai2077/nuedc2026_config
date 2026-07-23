#ifndef SQUARE_NAVIGATION_H_
#define SQUARE_NAVIGATION_H_

#include <stdint.h>

#define SQUARE_NAV_CORNER_CONFIRM_TICKS (2U)
#define SQUARE_NAV_STOP_TICKS           (10U)
#define SQUARE_NAV_IMU_FAILURE_LIMIT    (3U)
#define SQUARE_NAV_TURN_TIMEOUT_TICKS   (300U)
#define SQUARE_NAV_LEFT_TURN_DEG        (90.0f)

typedef enum {
    SQUARE_NAV_STATE_FOLLOW_ARMED = 0,
    SQUARE_NAV_STATE_STOPPING,
    SQUARE_NAV_STATE_TURNING,
    SQUARE_NAV_STATE_FOLLOW_WAIT_CLEAR,
    SQUARE_NAV_STATE_FAULT
} SQUARE_NAV_State;

typedef struct {
    int16_t rightTargetMmps;
    int16_t leftTargetMmps;
    SQUARE_NAV_State state;
    uint8_t rawMask;
    uint8_t cornerConfirmTicks;
    uint8_t stopTicks;
    uint8_t imuFailureTicks;
    uint16_t turnTicks;
    float currentYawDeg;
    float targetYawDeg;
} SQUARE_NAV_Output;

void SQUARE_NAV_init(void);
void SQUARE_NAV_update10ms(uint8_t rawMask, uint8_t yawValid,
    float currentYawDeg, SQUARE_NAV_Output *output);
void SQUARE_NAV_getOutput(SQUARE_NAV_Output *output);

#endif /* SQUARE_NAVIGATION_H_ */
