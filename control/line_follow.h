#ifndef LINE_FOLLOW_H_
#define LINE_FOLLOW_H_

#include <stdint.h>

#define LINE_FOLLOW_DEFAULT_KP          (5.0f)
#define LINE_FOLLOW_DEFAULT_KI          (0.0f)
#define LINE_FOLLOW_DEFAULT_KD          (0.08f)
#define LINE_FOLLOW_KP_MAX              (50.0f)
#define LINE_FOLLOW_KI_MAX              (20.0f)
#define LINE_FOLLOW_KD_MAX              (5.0f)
#define LINE_FOLLOW_CORRECTION_MAX_MMPS (150U)
#define LINE_FOLLOW_LOST_STOP_TICKS     (15U)

typedef enum {
    LINE_FOLLOW_STATE_TRACKING = 0,
    LINE_FOLLOW_STATE_INTERSECTION,
    LINE_FOLLOW_STATE_HOLD_LAST,
    LINE_FOLLOW_STATE_LOST
} LINE_FOLLOW_State;

typedef struct {
    uint8_t rawMask;
    uint8_t activeMask;
    uint8_t lostLineTicks;
    LINE_FOLLOW_State state;
    float kp;
    float ki;
    float kd;
    float errorMm;
    float integralMmS;
    float derivativeMmPerS;
    float proportionalMmps;
    float integralMmps;
    float derivativeMmps;
    float correctionMmps;
    int16_t curveBiasMmps;
    int16_t rightTargetMmps;
    int16_t leftTargetMmps;
} LINE_FOLLOW_Output;

void LINE_FOLLOW_init(void);
void LINE_FOLLOW_setBaseSpeedMmps(int16_t baseSpeedMmps);
void LINE_FOLLOW_setCurveBiasMmps(int16_t curveBiasMmps);
uint8_t LINE_FOLLOW_pidIsValid(float kp, float ki, float kd);
uint8_t LINE_FOLLOW_setPid(float kp, float ki, float kd);
void LINE_FOLLOW_resetDynamics(void);
void LINE_FOLLOW_update10ms(uint8_t rawMask, LINE_FOLLOW_Output *output);
void LINE_FOLLOW_getOutput(LINE_FOLLOW_Output *output);

#endif /* LINE_FOLLOW_H_ */
