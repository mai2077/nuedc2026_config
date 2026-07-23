#ifndef TURN_INDICATOR_H_
#define TURN_INDICATOR_H_

#include <stdint.h>

#define TURN_INDICATOR_BUZZER_TICKS (10U)
#define TURN_INDICATOR_LED_TICKS    (100U)

void TURN_INDICATOR_init(void);
void TURN_INDICATOR_start(void);
void TURN_INDICATOR_update10ms(void);
void TURN_INDICATOR_stop(void);

#endif /* TURN_INDICATOR_H_ */

