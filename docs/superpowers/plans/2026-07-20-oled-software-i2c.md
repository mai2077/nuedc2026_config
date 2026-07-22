# OLED Software-I2C Migration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Port the supplied SSD1306 128x64 OLED driver to MSPM0 software I2C on PB24/PA24 and show a startup smoke-test screen without changing the OLED pin assignment.

**Architecture:** `bsp/oled_soft_i2c.c` owns GPIO output-enable based open-drain signaling and byte transactions. `bsp/oled.c` owns SSD1306 commands, page addressing, the retained display API, and the migrated ASCII font in `bsp/oled_font.h`. `app/main.c` initializes the display once after SysConfig, shows four diagnostic lines, and keeps the existing motor-key loop.

**Tech Stack:** MSPM0G3507 DriverLib, SysConfig-generated `ti_msp_dl_config.h`, TI Arm Clang, C11 host tests with GCC, CCS Debug project.

---

### Task 1: Add Host Test Surface And Failing OLED Tests

**Files:**
- Modify: `tests/stubs/ti_msp_dl_config.h`
- Create: `tests/oled_host_test.c`
- Create: `bsp/oled_soft_i2c.h`
- Create: `bsp/oled.h`

- [x] **Step 1: Add the OLED GPIO symbols and DriverLib declarations to the host stub.**

Add `gTestGPIOA`, `gTestGPIOB`, `OLED_I2C_OLED_SCL_PORT`, `OLED_I2C_OLED_SCL_PIN`, `OLED_I2C_OLED_SDA_PORT`, `OLED_I2C_OLED_SDA_PIN`, `CPUCLK_FREQ`, and declarations for `DL_GPIO_enableOutput`, `DL_GPIO_disableOutput`, `DL_GPIO_setPins`, `DL_GPIO_clearPins`, and `delay_cycles` without changing existing test symbols.

Declare the compatibility-level protocol functions in `bsp/oled.h` so the host test and external callers do not rely on implicit declarations:

```c
void OLED_WriteCommand(uint8_t command);
void OLED_WriteData(uint8_t data);
void OLED_SetCursor(uint8_t page, uint8_t column);
```

- [x] **Step 2: Write the failing host test.**

Compile a recorder-backed test under `OLED_HOST_TEST` that calls `OLED_WriteCommand(0xAE)`, `OLED_WriteData(0x55)`, and `OLED_SetCursor(2, 0x34)`. Assert the protocol byte order:

```text
START, 0x78, 0x00, 0xAE, STOP
START, 0x78, 0x40, 0x55, STOP
START, 0x78, 0x00, 0xB2, STOP
START, 0x78, 0x00, 0x13, STOP
START, 0x78, 0x00, 0x04, STOP
```

Also assert that `OLED_I2C_Init()` releases both lines and that `OLED_ShowString(1, 1, "OK")` emits at least one data transaction.

- [x] **Step 3: Run the test and verify the expected RED result.**

Run:

```powershell
& 'C:\msys64\ucrt64\bin\gcc.exe' -std=c11 -Wall -Wextra -Werror `
  -DOLED_HOST_TEST -Itests/stubs -Ibsp `
  tests/oled_host_test.c bsp/oled_soft_i2c.c bsp/oled.c `
  -o tests/oled_host_test.exe
```

Expected: compilation fails because the OLED implementation and its DriverLib bindings do not exist yet.

### Task 2: Implement MSPM0 Open-Drain Software I2C

**Files:**
- Create: `bsp/oled_soft_i2c.c`
- Modify: `bsp/oled_soft_i2c.h`

- [x] **Step 1: Implement line primitives.**

Implement `OLED_I2C_Init`, `OLED_I2C_Start`, `OLED_I2C_Stop`, and `OLED_I2C_SendByte` using the generated ports and pins. Drive a low by clearing the latch before `DL_GPIO_enableOutput`; release high by `DL_GPIO_disableOutput`. Use `delay_cycles(OLED_I2C_DELAY_CYCLES)` between data/clock transitions. Send MSB first and provide the ninth clock without sampling ACK, matching the source driver.

- [x] **Step 2: Run the host test.**

Run the Task 1 GCC command. Expected output:

```text
oled host tests passed
```

### Task 3: Port SSD1306 Commands, Rendering, And Font

**Files:**
- Create: `bsp/oled.c`
- Create: `bsp/oled_font.h`
- Modify: `bsp/oled.h`

- [x] **Step 1: Move the font table.**

Copy the exact `OLED_F8x16` table from the supplied `OLED_Font.h` into `bsp/oled_font.h`, add `<stdint.h>`, and keep the table private to `oled.c` by including the font header only there.

- [x] **Step 2: Implement protocol and public API.**

Implement `OLED_WriteCommand`, `OLED_WriteData`, `OLED_SetCursor`, `OLED_Clear`, `OLED_ShowChar`, `OLED_ShowString`, `OLED_ShowNum`, `OLED_ShowSignedNum`, `OLED_ShowHexNum`, `OLED_ShowBinNum`, and the supplied SSD1306 initialization sequence. Use 1-based line/column coordinates, guard invalid lines/columns and characters outside the printable ASCII font range, and use unsigned magnitude handling for negative signed values.

- [x] **Step 3: Run the host test and rerun all existing host tests.**

Run the OLED test plus the existing TB6612, key, track/UART, I2C, IMU, protocol, and scheduler test commands. Expected: every executable exits 0 and the OLED test prints `oled host tests passed`.

### Task 4: Integrate Startup Display Without Disrupting Motor Test

**Files:**
- Modify: `app/main.c`

- [x] **Step 1: Add OLED startup calls.**

Include `oled.h`. Keep `MOTOR_POLARITY_TEST_init()` and `KEY_init()` immediately after `SYSCFG_DL_init()`, then call:

```c
OLED_Init();
OLED_ShowString(1, 1, "OLED TEST");
OLED_ShowString(2, 1, "MSPM0G3507");
OLED_ShowString(3, 1, "PB24/PA24");
OLED_ShowString(4, 1, "ADDR:3C");
```

Keep `MOTOR_POLARITY_TEST_init`, `KEY_init`, the 10 ms timer, and the existing main-loop behavior unchanged after startup.

- [x] **Step 2: Run TI Arm Clang syntax/type validation.**

Run:

```powershell
& 'C:\ti\ccs2100\ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\bin\tiarmclang.exe' `
  -fsyntax-only -D__MSPM0G3507__ -D__USE_SYSCONFIG__ `
  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb `
  -I. -IDebug -Ibsp -Iapp -Icontrol `
  -I'C:\ti\mspm0_sdk_2_10_00_04\source\third_party\CMSIS\Core\Include' `
  -I'C:\ti\mspm0_sdk_2_10_00_04\source' `
  bsp/oled_soft_i2c.c bsp/oled.c app/main.c
```

Expected: no diagnostics and exit code 0.

### Task 5: Regenerate/Validate SysConfig And CCS Source Inclusion

**Files:**
- Inspect only: `empty.syscfg`, `Debug/ti_msp_dl_config.c`, `Debug/ti_msp_dl_config.h`, `Debug/*.mk`

- [x] **Step 1: Run SysConfig static validation.**

Run:

```powershell
& 'C:\msys64\ucrt64\bin\python.exe' `
  'C:\Users\20778\.agents\skills\mspm0-ccs\scripts\check_syscfg.py' `
  'D:\MCU_Development\TI\MSPM0\nuedc2026_Pinconfig'
```

Confirm the checker reports PB24/PA24 and generated init symbols without warnings that affect OLED GPIO ownership.

- [ ] **Step 2: Refresh CCS and build.**

Refresh the project in CCS, clean, and build. If CCS does not include `bsp/oled_soft_i2c.c`, `bsp/oled.c`, or the new headers, refresh/reopen the project; do not edit `Debug/*.mk` by hand.

Current status: pending user-side CCS refresh. The generated `Debug/bsp/subdir_vars.mk` was inspected and does not yet list the two new OLED source files.

- [x] **Step 3: Record verification and hardware handoff.**

Record host/TI/SysConfig results in `progress.md`. Hardware validation is manual: flash, reset, confirm the four startup strings, then check address/wiring/pull-ups if blank. No automatic flashing is performed.

## Plan Self-Review

- The plan covers the approved design's GPIO open-drain behavior, retained API, font migration, startup display, host tests, TI syntax check, SysConfig validation, and CCS handoff.
- No generated file is modified by hand.
- The host test names and the public function signatures are consistent across tasks.
- ACK recovery and encoder integration remain explicitly outside this phase.
