#ifndef OLED_SOFT_I2C_H_
#define OLED_SOFT_I2C_H_

#include <stdint.h>

void OLED_I2C_Init(void);
void OLED_I2C_Start(void);
void OLED_I2C_Stop(void);
void OLED_I2C_SendByte(uint8_t byte);

#endif /* OLED_SOFT_I2C_H_ */
