#ifdef KEY_INPUT_HOST_TEST

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "ti_msp_dl_config.h"
#include "key.h"

#define KEY_ALL_PINS \
    (KEY_KEY1_PIN | KEY_KEY2_PIN | KEY_KEY3_PIN | KEY_KEY4_PIN)

GPIO_Regs gTestGPIOB;

static uint32_t gRawPins;

uint32_t DL_GPIO_readPins(GPIO_Regs *gpio, uint32_t pins)
{
    assert(gpio == KEY_PORT);
    return gRawPins & pins;
}

static void testActiveLowPacking(void)
{
    gRawPins = KEY_ALL_PINS;
    assert(KEY_readPressedMask() == 0U);

    gRawPins = KEY_ALL_PINS & ~KEY_KEY1_PIN;
    assert(KEY_readPressedMask() == KEY_MASK_KEY1);

    gRawPins = KEY_ALL_PINS & ~(KEY_KEY2_PIN | KEY_KEY4_PIN);
    assert(KEY_readPressedMask() == (KEY_MASK_KEY2 | KEY_MASK_KEY4));

    gRawPins = 0U;
    assert(KEY_readPressedMask() ==
           (KEY_MASK_KEY1 | KEY_MASK_KEY2 | KEY_MASK_KEY3 | KEY_MASK_KEY4));
}

static void testTwoSamplePressAndReleaseDebounce(void)
{
    uint8_t stable = 0xFFU;

    gRawPins = KEY_ALL_PINS;
    KEY_init();
    assert(!KEY_scan10ms(&stable));
    assert(!KEY_scan10ms(&stable));

    gRawPins = KEY_ALL_PINS & ~KEY_KEY1_PIN;
    assert(!KEY_scan10ms(&stable));
    assert(KEY_scan10ms(&stable));
    assert(stable == KEY_MASK_KEY1);
    assert(!KEY_scan10ms(&stable));

    gRawPins = KEY_ALL_PINS;
    assert(!KEY_scan10ms(&stable));
    assert(KEY_scan10ms(&stable));
    assert(stable == 0U);
}

static void testBounceAndNullOutputAreRejected(void)
{
    uint8_t stable = 0U;

    gRawPins = KEY_ALL_PINS;
    KEY_init();
    gRawPins &= ~KEY_KEY2_PIN;
    assert(!KEY_scan10ms(&stable));
    gRawPins |= KEY_KEY2_PIN;
    assert(!KEY_scan10ms(&stable));
    gRawPins &= ~KEY_KEY2_PIN;
    assert(!KEY_scan10ms(&stable));
    assert(KEY_scan10ms(&stable));
    assert(stable == KEY_MASK_KEY2);
    assert(!KEY_scan10ms(NULL));
}

int main(void)
{
    testActiveLowPacking();
    testTwoSamplePressAndReleaseDebounce();
    testBounceAndNullOutputAreRejected();
    puts("key input tests passed");
    return 0;
}

#endif /* KEY_INPUT_HOST_TEST */
