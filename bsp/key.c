#include "key.h"

#include <stddef.h>

#include "ti_msp_dl_config.h"

#define KEY_ALL_PINS \
    (KEY_KEY1_PIN | KEY_KEY2_PIN | KEY_KEY3_PIN | KEY_KEY4_PIN)
#define KEY_DEBOUNCE_SAMPLES (2U)

static uint8_t gCandidateMask;
static uint8_t gStableMask;
static uint8_t gConsecutiveSamples;

void KEY_init(void)
{
    gCandidateMask = 0U;
    gStableMask = 0U;
    gConsecutiveSamples = 0U;
}

uint8_t KEY_readPressedMask(void)
{
    uint32_t raw = DL_GPIO_readPins(KEY_PORT, KEY_ALL_PINS);
    uint8_t pressed = 0U;

    if ((raw & KEY_KEY1_PIN) == 0U) {
        pressed |= KEY_MASK_KEY1;
    }
    if ((raw & KEY_KEY2_PIN) == 0U) {
        pressed |= KEY_MASK_KEY2;
    }
    if ((raw & KEY_KEY3_PIN) == 0U) {
        pressed |= KEY_MASK_KEY3;
    }
    if ((raw & KEY_KEY4_PIN) == 0U) {
        pressed |= KEY_MASK_KEY4;
    }

    return pressed;
}

bool KEY_scan10ms(uint8_t *stableMask)
{
    uint8_t current;

    if (stableMask == NULL) {
        return false;
    }

    current = KEY_readPressedMask();
    if (current != gCandidateMask) {
        gCandidateMask = current;
        gConsecutiveSamples = 1U;
        return false;
    }

    if (gConsecutiveSamples < KEY_DEBOUNCE_SAMPLES) {
        ++gConsecutiveSamples;
    }
    if ((gConsecutiveSamples < KEY_DEBOUNCE_SAMPLES) ||
        (gStableMask == gCandidateMask)) {
        return false;
    }

    gStableMask = gCandidateMask;
    *stableMask = gStableMask;
    return true;
}
