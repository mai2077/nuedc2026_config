#ifndef BT_UART_H_
#define BT_UART_H_

#include <stdint.h>

void BT_UART_writeByte(uint8_t data);
void BT_UART_writeString(const char *text);
void BT_UART_writeUInt32(uint32_t value);
void BT_UART_writeInt32(int32_t value);
void BT_UART_writeSignedFixed3(int32_t milliValue);
void BT_UART_writeTrackFrame(uint8_t rawMask);
void BT_UART_initRxInterrupt(void);
uint8_t BT_UART_tryReadByte(uint8_t *data);
uint16_t BT_UART_getAndClearRxOverflowCount(void);

#endif /* BT_UART_H_ */
