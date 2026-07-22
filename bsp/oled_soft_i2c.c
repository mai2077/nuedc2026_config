#include "oled_soft_i2c.h"

#include "ti_msp_dl_config.h"

#define OLED_I2C_SPEED_HZ       (100000U)
#define OLED_I2C_DELAY_CYCLES   (CPUCLK_FREQ / (2U * OLED_I2C_SPEED_HZ))

static void oledI2cDelay(void)
{
    delay_cycles(OLED_I2C_DELAY_CYCLES);
}

static void oledI2cReleaseScl(void)
{
    DL_GPIO_disableOutput(OLED_I2C_OLED_SCL_PORT, OLED_I2C_OLED_SCL_PIN);
}

static void oledI2cDriveSclLow(void)
{
    DL_GPIO_clearPins(OLED_I2C_OLED_SCL_PORT, OLED_I2C_OLED_SCL_PIN);
    DL_GPIO_enableOutput(OLED_I2C_OLED_SCL_PORT, OLED_I2C_OLED_SCL_PIN);
}

static void oledI2cReleaseSda(void)
{
    DL_GPIO_disableOutput(OLED_I2C_OLED_SDA_PORT, OLED_I2C_OLED_SDA_PIN);
}

static void oledI2cDriveSdaLow(void)
{
    DL_GPIO_clearPins(OLED_I2C_OLED_SDA_PORT, OLED_I2C_OLED_SDA_PIN);
    DL_GPIO_enableOutput(OLED_I2C_OLED_SDA_PORT, OLED_I2C_OLED_SDA_PIN);
}

void OLED_I2C_Init(void)
{
    oledI2cReleaseScl();
    oledI2cReleaseSda();
    DL_GPIO_clearPins(OLED_I2C_OLED_SCL_PORT, OLED_I2C_OLED_SCL_PIN);
    DL_GPIO_clearPins(OLED_I2C_OLED_SDA_PORT, OLED_I2C_OLED_SDA_PIN);
    oledI2cDelay();
}

void OLED_I2C_Start(void)
{
    oledI2cReleaseSda();
    oledI2cReleaseScl();
    oledI2cDelay();
    oledI2cDriveSdaLow();
    oledI2cDelay();
    oledI2cDriveSclLow();
    oledI2cDelay();
}

void OLED_I2C_Stop(void)
{
    oledI2cDriveSdaLow();
    oledI2cDelay();
    oledI2cReleaseScl();
    oledI2cDelay();
    oledI2cReleaseSda();
    oledI2cDelay();
}

void OLED_I2C_SendByte(uint8_t byte)
{
    for (uint8_t bit = 0U; bit < 8U; ++bit) {
        if ((byte & (uint8_t)(0x80U >> bit)) != 0U) {
            oledI2cReleaseSda();
        } else {
            oledI2cDriveSdaLow();
        }
        oledI2cDelay();
        oledI2cReleaseScl();
        oledI2cDelay();
        oledI2cDriveSclLow();
        oledI2cDelay();
    }

    /* Release SDA for the ninth clock; ACK is intentionally not sampled. */
    oledI2cReleaseSda();
    oledI2cDelay();
    oledI2cReleaseScl();
    oledI2cDelay();
    oledI2cDriveSclLow();
    oledI2cDelay();
}
