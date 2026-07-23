#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdint.h>

typedef enum {
    DISPLAY_SOURCE_BLUETOOTH = 0,
    DISPLAY_SOURCE_DEBUG_UART = 1
} DISPLAY_Source;

void DISPLAY_reportReceivedLine(DISPLAY_Source source, const char *line);
void DISPLAY_reportPidDecoded(
    DISPLAY_Source source, uint16_t kpX100, uint16_t kiX100,
    uint16_t kdX100);
void DISPLAY_reportPidApplied(
    DISPLAY_Source source, uint16_t kpX100, uint16_t kiX100);
void DISPLAY_reportCommandRejected(
    DISPLAY_Source source, const char *reason);

#endif /* DISPLAY_H_ */
