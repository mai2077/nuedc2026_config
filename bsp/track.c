#include "ti_msp_dl_config.h"
#include "track.h"

#define TRACK3_ALL_PINS (TRACK3_OUT1_PIN | TRACK3_OUT2_PIN | \
    TRACK3_OUT3_PIN | TRACK3_OUT4_PIN | TRACK3_OUT5_PIN)

uint8_t TRACK_packRawPins(uint32_t gpioPins)
{
    uint8_t mask = 0U;

    if ((gpioPins & TRACK3_OUT1_PIN) != 0U) {
        mask |= TRACK_MASK_OUT1;
    }
    if ((gpioPins & TRACK3_OUT2_PIN) != 0U) {
        mask |= TRACK_MASK_OUT2;
    }
    if ((gpioPins & TRACK3_OUT3_PIN) != 0U) {
        mask |= TRACK_MASK_OUT3;
    }
    if ((gpioPins & TRACK3_OUT4_PIN) != 0U) {
        mask |= TRACK_MASK_OUT4;
    }
    if ((gpioPins & TRACK3_OUT5_PIN) != 0U) {
        mask |= TRACK_MASK_OUT5;
    }

    return mask;
}

uint8_t TRACK_readRawMask(void)
{
    uint32_t gpioPins = DL_GPIO_readPins(TRACK3_PORT, TRACK3_ALL_PINS);

    return TRACK_packRawPins(gpioPins);
}
