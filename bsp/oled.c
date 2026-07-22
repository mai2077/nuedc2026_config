#include "oled.h"

#include <stdbool.h>

#include "oled_font.h"
#include "oled_soft_i2c.h"
#include "ti_msp_dl_config.h"

#define OLED_WRITE_ADDRESS       (0x78U)
#define OLED_COMMAND_CONTROL    (0x00U)
#define OLED_DATA_CONTROL       (0x40U)
#define OLED_FONT_FIRST          (' ')
#define OLED_FONT_LAST           ('~')
#define OLED_PAGE_COUNT          (8U)
#define OLED_COLUMN_COUNT        (128U)
#define OLED_LINE_COUNT          (4U)
#define OLED_COLUMN_COUNT_8X16   (16U)

static uint32_t oledPow(uint32_t base, uint32_t exponent)
{
    uint32_t result = 1U;

    while (exponent != 0U) {
        result *= base;
        --exponent;
    }
    return result;
}

static bool oledCharacterIsPrintable(char character)
{
    return character >= OLED_FONT_FIRST && character <= OLED_FONT_LAST;
}

static void oledWriteDataBuffer(const uint8_t *data, uint8_t length)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(OLED_WRITE_ADDRESS);
    OLED_I2C_SendByte(OLED_DATA_CONTROL);
    for (uint8_t i = 0U; i < length; ++i) {
        OLED_I2C_SendByte(data[i]);
    }
    OLED_I2C_Stop();
}

void OLED_WriteCommand(uint8_t command)
{
    OLED_I2C_Start();
    OLED_I2C_SendByte(OLED_WRITE_ADDRESS);
    OLED_I2C_SendByte(OLED_COMMAND_CONTROL);
    OLED_I2C_SendByte(command);
    OLED_I2C_Stop();
}

void OLED_WriteData(uint8_t data)
{
    oledWriteDataBuffer(&data, 1U);
}

void OLED_SetCursor(uint8_t page, uint8_t column)
{
    if (page >= OLED_PAGE_COUNT || column >= OLED_COLUMN_COUNT) {
        return;
    }

    OLED_WriteCommand((uint8_t)(0xB0U | page));
    OLED_WriteCommand((uint8_t)(0x10U | ((column & 0xF0U) >> 4U)));
    OLED_WriteCommand((uint8_t)(column & 0x0FU));
}

void OLED_Clear(void)
{
    for (uint8_t page = 0U; page < OLED_PAGE_COUNT; ++page) {
        OLED_SetCursor(page, 0U);
        for (uint8_t column = 0U; column < OLED_COLUMN_COUNT; ++column) {
            OLED_WriteData(0x00U);
        }
    }
}

void OLED_ShowChar(uint8_t line, uint8_t column, char character)
{
    if (line == 0U || line > OLED_LINE_COUNT ||
        column == 0U || column > OLED_COLUMN_COUNT_8X16 ||
        !oledCharacterIsPrintable(character)) {
        return;
    }

    uint8_t fontIndex = (uint8_t)(character - OLED_FONT_FIRST);
    OLED_SetCursor((uint8_t)((line - 1U) * 2U),
        (uint8_t)((column - 1U) * 8U));
    oledWriteDataBuffer(&OLED_F8x16[fontIndex][0], 8U);

    OLED_SetCursor((uint8_t)((line - 1U) * 2U + 1U),
        (uint8_t)((column - 1U) * 8U));
    oledWriteDataBuffer(&OLED_F8x16[fontIndex][8], 8U);
}

void OLED_ShowString(uint8_t line, uint8_t column, const char *string)
{
    if (string == 0) {
        return;
    }

    for (uint8_t offset = 0U; string[offset] != '\0'; ++offset) {
        uint8_t currentColumn = (uint8_t)(column + offset);
        if (currentColumn > OLED_COLUMN_COUNT_8X16) {
            break;
        }
        OLED_ShowChar(line, currentColumn, string[offset]);
    }
}

void OLED_ShowNum(uint8_t line, uint8_t column, uint32_t number,
    uint8_t length)
{
    for (uint8_t i = 0U; i < length; ++i) {
        uint32_t divisor = oledPow(10U, (uint32_t)(length - i - 1U));
        OLED_ShowChar(line, (uint8_t)(column + i),
            (char)('0' + (number / divisor) % 10U));
    }
}

void OLED_ShowSignedNum(uint8_t line, uint8_t column, int32_t number,
    uint8_t length)
{
    uint32_t magnitude;

    OLED_ShowChar(line, column, number < 0 ? '-' : '+');
    if (number < 0) {
        magnitude = (uint32_t)(-(number + 1)) + 1U;
    } else {
        magnitude = (uint32_t)number;
    }

    for (uint8_t i = 0U; i < length; ++i) {
        uint32_t divisor = oledPow(10U, (uint32_t)(length - i - 1U));
        OLED_ShowChar(line, (uint8_t)(column + i + 1U),
            (char)('0' + (magnitude / divisor) % 10U));
    }
}

void OLED_ShowFixed2(uint8_t line, uint8_t column,
    uint16_t valueX100, uint8_t integerDigits)
{
    OLED_ShowNum(line, column, valueX100 / 100U, integerDigits);
    OLED_ShowChar(line, (uint8_t)(column + integerDigits), '.');
    OLED_ShowNum(line, (uint8_t)(column + integerDigits + 1U),
        valueX100 % 100U, 2U);
}

void OLED_ShowHexNum(uint8_t line, uint8_t column, uint32_t number,
    uint8_t length)
{
    for (uint8_t i = 0U; i < length; ++i) {
        uint8_t digit = (uint8_t)((number /
            oledPow(16U, (uint32_t)(length - i - 1U))) % 16U);
        char character = digit < 10U ? (char)('0' + digit) :
            (char)('A' + digit - 10U);
        OLED_ShowChar(line, (uint8_t)(column + i), character);
    }
}

void OLED_ShowBinNum(uint8_t line, uint8_t column, uint32_t number,
    uint8_t length)
{
    for (uint8_t i = 0U; i < length; ++i) {
        uint32_t divisor = oledPow(2U, (uint32_t)(length - i - 1U));
        OLED_ShowChar(line, (uint8_t)(column + i),
            (char)('0' + (number / divisor) % 2U));
    }
}

void OLED_Init(void)
{
    delay_cycles(CPUCLK_FREQ / 10U);
    OLED_I2C_Init();

    OLED_WriteCommand(0xAEU);
    OLED_WriteCommand(0xD5U);
    OLED_WriteCommand(0x80U);
    OLED_WriteCommand(0xA8U);
    OLED_WriteCommand(0x3FU);
    OLED_WriteCommand(0xD3U);
    OLED_WriteCommand(0x00U);
    OLED_WriteCommand(0x40U);
    OLED_WriteCommand(0xA1U);
    OLED_WriteCommand(0xC8U);
    OLED_WriteCommand(0xDAU);
    OLED_WriteCommand(0x12U);
    OLED_WriteCommand(0x81U);
    OLED_WriteCommand(0xCFU);
    OLED_WriteCommand(0xD9U);
    OLED_WriteCommand(0xF1U);
    OLED_WriteCommand(0xDBU);
    OLED_WriteCommand(0x30U);
    OLED_WriteCommand(0xA4U);
    OLED_WriteCommand(0xA6U);
    OLED_WriteCommand(0x8DU);
    OLED_WriteCommand(0x14U);
    OLED_WriteCommand(0xAFU);
    OLED_Clear();
}
