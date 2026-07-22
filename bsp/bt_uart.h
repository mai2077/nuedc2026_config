#ifndef BT_UART_H_
#define BT_UART_H_

#include <stdint.h>

void BT_UART_writeByte(uint8_t data);
void BT_UART_writeString(const char *text);
void BT_UART_writeTrackFrame(uint8_t rawMask);

#endif /* BT_UART_H_ */
