# MSPM0 OLED Software-I2C Migration Design

## Goal

Port the supplied STM32 SSD1306-compatible 128x64 OLED driver to the MSPM0G3507 project. The first target is a physical display smoke test using the existing `OLED_I2C` GPIO group and the module's external I2C pull-up resistors.

The port must preserve the useful public display API from the STM32 example while replacing STM32 GPIO and delay dependencies with MSPM0 DriverLib calls.

## Confirmed Hardware

- OLED controller/module: SSD1306-compatible 128x64 display.
- `OLED_SCL`: PB24.
- `OLED_SDA`: PA24.
- The module already provides external pull-up resistors on both I2C lines.
- The current SysConfig group is named `OLED_I2C`; no pin or peripheral reassignment is required.
- The source example uses the 8-bit write address `0x78`, equivalent to 7-bit address `0x3C`.

## Architecture

### `bsp/oled_soft_i2c.h` and `bsp/oled_soft_i2c.c`

Own the GPIO-level I2C transaction. The public interface is intentionally small:

```c
void OLED_I2C_Init(void);
void OLED_I2C_Start(void);
void OLED_I2C_Stop(void);
void OLED_I2C_SendByte(uint8_t byte);
```

The implementation emulates open-drain behavior with the MSPM0 GPIO output-enable bit:

- Logic low: clear the output latch, then enable the output driver.
- Logic high: disable the output driver and let the external pull-up raise the line.

The driver does not force a push-pull high level. It does not read ACK bits in this first smoke-test port, matching the supplied example's behavior. A later revision can add ACK and clock-stretch handling without changing the OLED rendering API.

The initial software-I2C clock target is approximately 100 kHz. Timing uses the generated `CPUCLK_FREQ` and a small configurable delay macro; it does not use STM32 HAL delays or busy loops whose duration depends on an unknown compiler optimization.

### `bsp/oled.h` and `bsp/oled.c`

Own the SSD1306 command/data protocol and retain these source-compatible functions:

```c
void OLED_Init(void);
void OLED_Clear(void);
void OLED_ShowChar(uint8_t line, uint8_t column, char character);
void OLED_ShowString(uint8_t line, uint8_t column, const char *string);
void OLED_ShowNum(uint8_t line, uint8_t column, uint32_t number, uint8_t length);
void OLED_ShowSignedNum(uint8_t line, uint8_t column, int32_t number, uint8_t length);
void OLED_ShowHexNum(uint8_t line, uint8_t column, uint32_t number, uint8_t length);
void OLED_ShowBinNum(uint8_t line, uint8_t column, uint32_t number, uint8_t length);
```

The driver uses the SSD1306 control bytes `0x00` for commands and `0x40` for display data, preserves page/column addressing, and keeps the supplied 8x16 ASCII font behavior. The font table is moved into `bsp/oled_font.h` and included only by the OLED implementation so it is not multiply defined.

The port will retain the example's 1-based line and column coordinates and validate character bounds before indexing the font table. The initialization sequence remains the supplied SSD1306 sequence, followed by a full clear.

### Application smoke test

`app/main.c` will force the motors stopped and initialize the keys immediately after `SYSCFG_DL_init()`. It then calls `OLED_Init()` and shows static text such as:

```text
OLED TEST
MSPM0G3507
PB24/PA24
ADDR:3C
```

The existing press-and-hold motor polarity test remains in the application loop. OLED rendering is performed only during startup, so it does not run from an ISR and does not disturb the 10 ms key-scan timing.

## Safety And Error Handling

- At startup, the generated GPIO state is released high before the first software-I2C transaction.
- Every transaction ends with a STOP condition, including command and data writes.
- No blocking OLED operation is called from an interrupt handler.
- This first port has no ACK/error return because the supplied API is `void` and the requested purpose is a display smoke test. A blank display is therefore diagnosed with power, ground, address, pull-up, and wiring checks before adding protocol recovery.
- `OLED_Clear()` and text rendering remain blocking operations; they are only used during startup in this test stage.

## Verification

### Host tests

Use a host-only GPIO/I2C recorder to verify:

- open-drain low/release-high transitions;
- command transaction address/control/data ordering;
- data transaction address/control/data ordering;
- SSD1306 cursor command bytes;
- numeric and string APIs do not write outside their requested coordinate range.

### Target checks

1. Run the MSPM0 SysConfig static checker. No generated file is edited by hand.
2. Run TI Arm Clang syntax/type validation for the new BSP files and `app/main.c`.
3. Refresh the CCS project if the new source files are not in its source list, then perform a normal Debug build. Do not edit `Debug/*.mk` manually.

### Hardware smoke test

1. Connect OLED VCC, GND, SCL, SDA, and confirm both pull-ups go to the MSPM0 logic voltage.
2. Flash the test application and reset the board.
3. Confirm the four text lines are stable and readable.
4. If the display remains blank, check the module's actual 7-bit address (`0x3C` versus `0x3D`), connector orientation, common ground, and pull-up voltage before changing the rendering code.

## Scope Boundaries

This OLED phase does not change the existing hardware I2C controller, add OLED scrolling/graphics, or add ACK recovery. Encoder values are now supplied by the separate encoder test integration.
