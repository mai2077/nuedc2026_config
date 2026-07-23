#ifndef PI_TUNER_H_
#define PI_TUNER_H_

#include <stdint.h>

#include "control/wheel_speed.h"

#ifndef PI_TUNER_ENABLE_BT_TELEMETRY
#define PI_TUNER_ENABLE_BT_TELEMETRY (1U)
#endif

void PI_TUNER_init(void);
void PI_TUNER_pollUart(void);
void PI_TUNER_update10ms(uint32_t tick10ms);

uint8_t PI_TUNER_isRunning(void);

#endif /* PI_TUNER_H_ */
