#ifndef OLED_H_
#define OLED_H_

#include <stdint.h>

void OLED_Init(void);
void OLED_Clear(void);
void OLED_WriteCommand(uint8_t command);
void OLED_WriteData(uint8_t data);
void OLED_SetCursor(uint8_t page, uint8_t column);
void OLED_ShowChar(uint8_t line, uint8_t column, char character);
void OLED_ShowString(uint8_t line, uint8_t column, const char *string);
void OLED_ShowNum(uint8_t line, uint8_t column, uint32_t number, uint8_t length);
void OLED_ShowSignedNum(uint8_t line, uint8_t column, int32_t number, uint8_t length);
void OLED_ShowFixed2(uint8_t line, uint8_t column,
    uint16_t valueX100, uint8_t integerDigits);
void OLED_ShowHexNum(uint8_t line, uint8_t column, uint32_t number, uint8_t length);
void OLED_ShowBinNum(uint8_t line, uint8_t column, uint32_t number, uint8_t length);

#endif /* OLED_H_ */
