#ifndef TRACK_H_
#define TRACK_H_

#include <stdint.h>

#define TRACK_MASK_OUT1 (1U << 0)
#define TRACK_MASK_OUT2 (1U << 1)
#define TRACK_MASK_OUT3 (1U << 2)
#define TRACK_MASK_OUT4 (1U << 3)
#define TRACK_MASK_OUT5 (1U << 4)
#define TRACK_MASK_ALL  (0x1FU)

uint8_t TRACK_packRawPins(uint32_t gpioPins);
uint8_t TRACK_readRawMask(void);

#endif /* TRACK_H_ */
