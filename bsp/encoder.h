#ifndef ENCODER_H_
#define ENCODER_H_

#include <stdint.h>

void ENCODER_init(void);
void ENCODER_reset(void);
void ENCODER_updateSpeed10ms(void);

int32_t ENCODER_getRightPosition(void);
int32_t ENCODER_getLeftPosition(void);
int32_t ENCODER_getRightSpeed10ms(void);
int32_t ENCODER_getLeftSpeed10ms(void);

#endif /* ENCODER_H_ */
