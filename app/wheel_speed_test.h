#ifndef WHEEL_SPEED_TEST_H_
#define WHEEL_SPEED_TEST_H_

#include <stdint.h>

void WHEEL_SPEED_TEST_init(void);
void WHEEL_SPEED_TEST_applyKeys(uint8_t pressedMask);
void WHEEL_SPEED_TEST_update10ms(void);

#endif /* WHEEL_SPEED_TEST_H_ */
