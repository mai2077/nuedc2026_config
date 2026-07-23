#ifndef DEBUG_UART_H_
#define DEBUG_UART_H_

#include <stdint.h>

void DEBUG_UART_writeByte(uint8_t data);
void DEBUG_UART_writeString(const char *text);
void DEBUG_UART_writeUInt32(uint32_t value);
void DEBUG_UART_writeInt32(int32_t value);
void DEBUG_UART_writeHexByte(uint8_t value);
void DEBUG_UART_writeSignedFixed3(int32_t milliValue);
void DEBUG_UART_writeTrackFrame(uint8_t rawMask);
void DEBUG_UART_initRxInterrupt(void);
uint8_t DEBUG_UART_tryReadByte(uint8_t *data);

#endif /* DEBUG_UART_H_ */
