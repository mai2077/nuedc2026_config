#ifndef PI_TUNER_H_
#define PI_TUNER_H_

#include <stdint.h>

#include "control/wheel_speed.h"

#ifndef PI_TUNER_ENABLE_BT_TELEMETRY
#define PI_TUNER_ENABLE_BT_TELEMETRY (0U)
#endif

#ifndef PI_TUNER_ENABLE_BT_COMMANDS
#define PI_TUNER_ENABLE_BT_COMMANDS (0U)
#endif

void PI_TUNER_init(void);
void PI_TUNER_pollUart(void);
void PI_TUNER_update10ms(uint32_t tick10ms);

uint8_t PI_TUNER_isRunning(void);
int16_t PI_TUNER_getCurveBiasMmps(void);
uint32_t PI_TUNER_getCurveBiasRevision(void);

#endif /* PI_TUNER_H_ */
