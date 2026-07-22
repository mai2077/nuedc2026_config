#ifdef OLED_HOST_TEST

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "oled.h"
#include "oled_font.h"
#include "oled_soft_i2c.h"
#include "ti_msp_dl_config.h"

GPIO_Regs gTestGPIOA;
GPIO_Regs gTestGPIOB;

typedef struct {
    bool outputEnabled;
    bool latchHigh;
} TestLine;

static TestLine gScl;
static TestLine gSda;
static bool gSclLevel;
static bool gSdaLevel;
static bool gActive;
static uint8_t gByte;
static uint8_t gBitCount;
static bool gAckSlot;
static uint8_t gBytes[1024];
static uint16_t gByteCount;

static TestLine *lineFor(GPIO_Regs *gpio, uint32_t pins)
{
    if (gpio == OLED_I2C_OLED_SCL_PORT && pins == OLED_I2C_OLED_SCL_PIN) {
        return &gScl;
    }
    assert(gpio == OLED_I2C_OLED_SDA_PORT && pins == OLED_I2C_OLED_SDA_PIN);
    return &gSda;
}

static void sampleBus(void)
{
    bool oldScl = gSclLevel;
    bool oldSda = gSdaLevel;

    gSclLevel = !gScl.outputEnabled || gScl.latchHigh;
    gSdaLevel = !gSda.outputEnabled || gSda.latchHigh;

    if (gSclLevel && oldSda && !gSdaLevel) {
        gActive = true;
        gByte = 0U;
        gBitCount = 0U;
        gAckSlot = false;
    } else if (gSclLevel && !oldSda && gSdaLevel) {
        gActive = false;
    }

    if (gActive && gSclLevel && !oldScl) {
        if (gAckSlot) {
            gAckSlot = false;
        } else {
            gByte = (uint8_t)((gByte << 1) | (gSdaLevel ? 1U : 0U));
            ++gBitCount;
            if (gBitCount == 8U) {
                assert(gByteCount < sizeof(gBytes));
                gBytes[gByteCount++] = gByte;
                gByte = 0U;
                gBitCount = 0U;
                gAckSlot = true;
            }
        }
    }

    if (!gSclLevel && oldScl && gSdaLevel != oldSda) {
        if (gSdaLevel) {
            gActive = false;
        }
    }
}

static void resetRecorder(void)
{
    gScl = (TestLine){false, false};
    gSda = (TestLine){false, false};
    gSclLevel = true;
    gSdaLevel = true;
    gActive = false;
    gByte = 0U;
    gBitCount = 0U;
    gAckSlot = false;
    gByteCount = 0U;
}

void DL_GPIO_setPins(GPIO_Regs *gpio, uint32_t pins)
{
    lineFor(gpio, pins)->latchHigh = true;
    sampleBus();
}

void DL_GPIO_clearPins(GPIO_Regs *gpio, uint32_t pins)
{
    lineFor(gpio, pins)->latchHigh = false;
    sampleBus();
}

void DL_GPIO_enableOutput(GPIO_Regs *gpio, uint32_t pins)
{
    lineFor(gpio, pins)->outputEnabled = true;
    sampleBus();
}

void DL_GPIO_disableOutput(GPIO_Regs *gpio, uint32_t pins)
{
    lineFor(gpio, pins)->outputEnabled = false;
    sampleBus();
}

void delay_cycles(uint32_t cycles)
{
    (void)cycles;
}

static void expectBytes(const uint8_t *expected, uint16_t length)
{
    if (gByteCount != length) {
        fprintf(stderr, "recorded %u bytes, expected %u\n",
            (unsigned)gByteCount, (unsigned)length);
        for (uint16_t i = 0U; i < gByteCount; ++i) {
            fprintf(stderr, "%02X ", gBytes[i]);
        }
        fputc('\n', stderr);
    }
    assert(gByteCount == length);
    for (uint16_t i = 0U; i < length; ++i) {
        assert(gBytes[i] == expected[i]);
    }
}

static void testOpenDrainRelease(void)
{
    resetRecorder();
    OLED_I2C_Init();
    assert(!gScl.outputEnabled);
    assert(!gSda.outputEnabled);
}

static void testCommandDataAndCursorTransactions(void)
{
    static const uint8_t expected[] = {
        0x78U, 0x00U, 0xAEU,
        0x78U, 0x40U, 0x55U,
        0x78U, 0x00U, 0xB2U,
        0x78U, 0x00U, 0x13U,
        0x78U, 0x00U, 0x04U,
    };

    resetRecorder();
    OLED_WriteCommand(0xAEU);
    OLED_WriteData(0x55U);
    OLED_SetCursor(2U, 0x34U);
    expectBytes(expected, (uint8_t)sizeof(expected));
}

static void testStringWritesData(void)
{
    resetRecorder();
    OLED_ShowString(1U, 1U, "OK");
    assert(gByteCount > 3U);
    assert(gBytes[0] == 0x78U);
    assert(gBytes[1] == 0x00U);
}

static void testCharacterUsesBufferedDataTransactions(void)
{
    resetRecorder();
    OLED_ShowChar(1U, 1U, 'A');
    assert(gByteCount == 38U);
}

static void expectRenderedCharacter(uint16_t byteOffset, char character)
{
    uint8_t fontIndex = (uint8_t)(character - ' ');

    for (uint8_t i = 0U; i < 8U; ++i) {
        assert(gBytes[byteOffset + 11U + i] ==
            OLED_F8x16[fontIndex][i]);
        assert(gBytes[byteOffset + 30U + i] ==
            OLED_F8x16[fontIndex][i + 8U]);
    }
}

static void testFixed2RendersZeroPaddedValues(void)
{
    static const char first[] = "020.50";
    static const char second[] = "001.75";

    resetRecorder();
    OLED_ShowFixed2(1U, 1U, 2050U, 3U);
    assert(gByteCount == 228U);
    for (uint8_t i = 0U; i < 6U; ++i) {
        expectRenderedCharacter((uint16_t)i * 38U, first[i]);
    }

    resetRecorder();
    OLED_ShowFixed2(1U, 1U, 175U, 3U);
    assert(gByteCount == 228U);
    for (uint8_t i = 0U; i < 6U; ++i) {
        expectRenderedCharacter((uint16_t)i * 38U, second[i]);
    }
}

int main(void)
{
    testOpenDrainRelease();
    testCommandDataAndCursorTransactions();
    testStringWritesData();
    testCharacterUsesBufferedDataTransactions();
    testFixed2RendersZeroPaddedValues();
    puts("oled host tests passed");
    return 0;
}

#endif /* OLED_HOST_TEST */
