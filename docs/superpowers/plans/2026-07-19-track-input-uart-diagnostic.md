# Track Input UART Diagnostic Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Read five raw track inputs into a stable bit mask and print `TRACK mask=0xNN bits=NNNNN` through the configured `RESV1_UART` at 10 Hz.

**Architecture:** `bsp/track` owns one-shot GPIOB sampling and raw bit packing. `bsp/debug_uart` owns blocking UART0 output and frame formatting without `printf`. `app/main.c` keeps the motors stopped and runs the diagnostic loop. Host tests replace generated hardware definitions with a small stub so mapping and output bytes can be verified without a board.

**Tech Stack:** C11, TI MSPM0 DriverLib, SysConfig-generated names, TI Arm Clang/CCS, host GCC for logic tests.

---

### Task 1: Add failing host tests

**Files:**
- Create: `tests/stubs/ti_msp_dl_config.h`
- Create: `tests/track_uart_diag_test.c`

- [x] **Step 1: Create the DriverLib stub header**

Define `GPIO_Regs`, fake GPIOB/UART0 objects, the five non-overlapping `TRACK3_OUTx_PIN` values, `TRACK3_PORT`, `RESV1_UART_INST`, and declarations for `DL_GPIO_readPins()` and `DL_UART_Main_transmitDataBlocking()`.

- [x] **Step 2: Create mapping and UART frame tests**

The test must assert:

```c
TRACK_packRawPins(TRACK3_OUT1_PIN) == 0x01U;
TRACK_packRawPins(TRACK3_OUT2_PIN) == 0x02U;
TRACK_packRawPins(TRACK3_OUT3_PIN) == 0x04U;
TRACK_packRawPins(TRACK3_OUT4_PIN) == 0x08U;
TRACK_packRawPins(TRACK3_OUT5_PIN) == 0x10U;
TRACK_packRawPins(allPins) == 0x1FU;
TRACK_readRawMask() == 0x12U; /* OUT5 + OUT2 */
```

Capture UART bytes in the transmit stub and assert:

```c
DEBUG_UART_writeTrackFrame(0x05U);
assert(strcmp(txBuffer, "TRACK mask=0x05 bits=00101\r\n") == 0);
```

- [x] **Step 3: Run the tests and verify RED**

Run:

```powershell
& 'C:\msys64\ucrt64\bin\gcc.exe' -std=c11 -Wall -Wextra -Werror `
  -DTRACK_DIAG_HOST_TEST `
  -Itests/stubs -Ibsp tests/track_uart_diag_test.c `
  bsp/track.c bsp/debug_uart.c -o tests/track_uart_diag_test.exe
```

Expected result: compilation fails because `bsp/track.c`, `bsp/track.h`, `bsp/debug_uart.c`, and `bsp/debug_uart.h` do not exist yet.

### Task 2: Implement raw track input packing

**Files:**
- Create: `bsp/track.h`
- Create: `bsp/track.c`

- [x] **Step 1: Define the public mask contract**

Expose:

```c
#define TRACK_MASK_OUT1 (1U << 0)
#define TRACK_MASK_OUT2 (1U << 1)
#define TRACK_MASK_OUT3 (1U << 2)
#define TRACK_MASK_OUT4 (1U << 3)
#define TRACK_MASK_OUT5 (1U << 4)
#define TRACK_MASK_ALL  (0x1FU)

uint8_t TRACK_packRawPins(uint32_t gpioPins);
uint8_t TRACK_readRawMask(void);
```

- [x] **Step 2: Implement one-read GPIO sampling**

`TRACK_packRawPins()` maps the generated `TRACK3_OUT1_PIN` through `TRACK3_OUT5_PIN` values to diagnostic bits 0 through 4. `TRACK_readRawMask()` calls `DL_GPIO_readPins()` exactly once with the OR of all five physical pin masks and passes that value to `TRACK_packRawPins()`.

### Task 3: Implement blocking diagnostic UART output

**Files:**
- Create: `bsp/debug_uart.h`
- Create: `bsp/debug_uart.c`

- [x] **Step 1: Define the UART output API**

Expose:

```c
void DEBUG_UART_writeByte(uint8_t data);
void DEBUG_UART_writeString(const char *text);
void DEBUG_UART_writeTrackFrame(uint8_t rawMask);
```

- [x] **Step 2: Implement DriverLib blocking output**

`DEBUG_UART_writeByte()` calls `DL_UART_Main_transmitDataBlocking(RESV1_UART_INST, data)`. `DEBUG_UART_writeString()` emits bytes until the null terminator. `DEBUG_UART_writeTrackFrame()` masks the input with `0x1F`, emits two uppercase hexadecimal digits, then emits bits 4 through 0 and `\r\n`.

- [x] **Step 3: Run the host tests and verify GREEN**

Compile with the command from Task 1, including `-DTRACK_DIAG_HOST_TEST`, then run:

```powershell
& '.\tests\track_uart_diag_test.exe'
```

Expected result: exit code 0 and output `track/uart diagnostic tests passed`.

### Task 4: Integrate the diagnostic loop

**Files:**
- Modify: `app/main.c`

- [x] **Step 1: Keep startup safe**

Retain `SYSCFG_DL_init()` followed by `TB6612_init()`. Remove the four commented motor polarity-test calls from the active diagnostic application.

- [x] **Step 2: Add the 10 Hz diagnostic loop**

Include `debug_uart.h` and `track.h`, print:

```c
DEBUG_UART_writeString(
    "TRACK DIAG bit4..bit0=OUT5..OUT1 raw GPIO levels\r\n");
```

Then repeatedly call `DEBUG_UART_writeTrackFrame(TRACK_readRawMask())` followed by `delay_cycles(CPUCLK_FREQ / 10U)`.

### Task 5: Verify configuration and source integration

**Files:**
- Inspect: `empty.syscfg`
- Inspect: `Debug/ti_msp_dl_config.h`
- Inspect: `Debug/ti_msp_dl_config.c`

- [x] **Step 1: Run host tests fresh**

Run the compile and test commands from Task 3 and require exit code 0.

- [x] **Step 2: Run the MSPM0 static configuration check**

Run:

```powershell
python C:\Users\20778\.agents\skills\mspm0-ccs\scripts\check_syscfg.py .
```

Expected result: SysConfig metadata, generated init spelling, and pin checks report OK. Informational ADC/retention messages are not UART failures.

- [x] **Step 3: Confirm source invariants**

Confirm one `main()` definition, all five TRACK3 generated pins, `RESV1_UART_INST`, PA10 TX, PA11 RX, and no modifications under `Debug/`.

- [x] **Step 4: Document on-board validation**

Provide wiring and terminal settings, then verify OUT1 through OUT5 one at a time, combinations by bitwise OR, and the all-low/all-high masks. State separately that static/build success does not prove electrical behavior.
