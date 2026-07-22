#ifndef TB6612_H_
#define TB6612_H_

#include <stdint.h>

#define TB6612_PWM_MAX  (1000)
#define TB6612_TEST_DUTY (150)

/* Positive is software-defined forward; negative is software-defined reverse. */
void TB6612_init(void);
void TB6612_setMotor1(int16_t signedPwm);
void TB6612_setMotor2(int16_t signedPwm);
void TB6612_stopAll(void);

#endif /* TB6612_H_ */
