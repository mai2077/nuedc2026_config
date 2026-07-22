#ifndef KEY_H_
#define KEY_H_

#include <stdbool.h>
#include <stdint.h>

#define KEY_MASK_KEY1 (1U << 0)
#define KEY_MASK_KEY2 (1U << 1)
#define KEY_MASK_KEY3 (1U << 2)
#define KEY_MASK_KEY4 (1U << 3)

void KEY_init(void);
uint8_t KEY_readPressedMask(void);
bool KEY_scan10ms(uint8_t *stableMask);

#endif /* KEY_H_ */
