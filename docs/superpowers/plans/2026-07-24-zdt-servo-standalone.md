# ZDT EMM_V5 Standalone Motor Test Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Port the ZDT EMM_V5 UART protocol to MSPM0G3507 and provide a safe RESV1-commanded standalone test for one motor on STEP1_UART.

**Architecture:** `bsp/bsp_ZDT_Servo_emm_v5.c` owns pure frame construction and blocking STEP1_UART transmission. `app/ZDT_Servo_test.c` owns command parsing and RESV1 diagnostics. `app/main.c` temporarily becomes a minimal serial-motor test entry point so no car motor or line-follow subsystem is initialized.

**Tech Stack:** MSPM0G3507 DriverLib, SysConfig-generated STEP1_UART/RESV1_UART symbols, TI Arm Clang 5.1.1 LTS, C11 host tests with GCC, CCS GNU Make build.

---

## File Structure

- Create `bsp/bsp_ZDT_Servo_emm_v5.h`: protocol constants, enums and public compatibility APIs.
- Create `bsp/bsp_ZDT_Servo_emm_v5.c`: frame builders, last-frame snapshot and STEP1_UART transmitter.
- Create `app/ZDT_Servo_test.h`: standalone test defaults and public lifecycle API.
- Create `app/ZDT_Servo_test.c`: RESV1 line parser, fixed test actions and diagnostics.
- Modify `app/main.c`: activate only SysConfig, RESV1 RX and the ZDT test loop.
- Create `tests/zdt_servo_protocol_test.c`: protocol-frame unit tests.
- Create `tests/zdt_servo_command_test.c`: command-parser unit tests with BSP/UART stubs.
- Modify `tests/stubs/ti_msp_dl_config.h`: add minimal STEP1 UART symbols needed by host tests.

## Task 1: Add Protocol Frame Tests And Public API

**Files:**
- Create: `tests/zdt_servo_protocol_test.c`
- Create: `bsp/bsp_ZDT_Servo_emm_v5.h`

- [ ] **Step 1: Declare the portable protocol API.**

Define `BSP_EMM_V5_FRAME_END` as `0x6B`; define quick-position modes as
`REL_LAST=0`, `ABSOLUTE=1`, `REL_CURRENT=2`; define regular position modes as
`RELATIVE=0`, `ABSOLUTE=1`; then declare readable system parameters and:

```c
typedef enum {
    BSP_ZDT_SERVO_OK = 0,
    BSP_ZDT_SERVO_INVALID_ARGUMENT,
    BSP_ZDT_SERVO_TX_ERROR
} BSP_ZDT_Servo_Status;

size_t BSP_EmmV5_Build_SetQPosParams(uint8_t addr, uint16_t vel,
    uint8_t acc, uint8_t raF, bool snF, uint8_t *buf, size_t bufSize);
size_t BSP_EmmV5_Build_QPosControl(uint8_t addr, int32_t clk,
    uint8_t *buf, size_t bufSize);
size_t BSP_EmmV5_Build_VelControl(uint8_t addr, uint8_t dir,
    uint16_t vel, uint8_t acc, bool snF, uint8_t *buf, size_t bufSize);
size_t BSP_EmmV5_Build_PosControl(uint8_t addr, uint8_t dir,
    uint16_t vel, uint8_t acc, uint32_t clk, uint8_t raF, bool snF,
    uint8_t *buf, size_t bufSize);
size_t BSP_EmmV5_Build_StopNow(uint8_t addr, bool snF,
    uint8_t *buf, size_t bufSize);
size_t BSP_EmmV5_Build_SynchronousMotion(uint8_t addr,
    uint8_t *buf, size_t bufSize);
size_t BSP_EmmV5_Build_ReadSysParam(uint8_t addr,
    BSPEmmV5SysParam param, uint8_t *buf, size_t bufSize);
```

- [ ] **Step 2: Write failing builder tests.**

Assert representative frames exactly:

```c
assert_frame(BSP_EmmV5_Build_StopNow(0x01U, false, frame, sizeof(frame)),
    frame, (uint8_t[]){0x01U, 0xFEU, 0x98U, 0x00U, 0x6BU}, 5U);
assert_frame(BSP_EmmV5_Build_QPosControl(0x01U, 3200, frame, sizeof(frame)),
    frame, (uint8_t[]){0x01U, 0xFCU, 0x00U, 0x00U, 0x0CU, 0x80U, 0x6BU}, 7U);
assert_frame(BSP_EmmV5_Build_QPosControl(0x01U, -3200, frame, sizeof(frame)),
    frame, (uint8_t[]){0x01U, 0xFCU, 0xFFU, 0xFFU, 0xF3U, 0x80U, 0x6BU}, 7U);
```

Also cover FWD/REV direction bytes, quick-position parameter frame, buffer-too-small and null-buffer cases.

- [ ] **Step 3: Compile and verify RED.**

```powershell
& 'C:\msys64\ucrt64\bin\gcc.exe' -std=c11 -Wall -Wextra -Werror `
  -Ibsp tests/zdt_servo_protocol_test.c bsp/bsp_ZDT_Servo_emm_v5.c `
  -o tests/zdt_servo_protocol_test.exe
```

Expected: compilation fails because the implementation does not exist.

## Task 2: Implement EMM_V5 Builders And STEP1 Transmission

**Files:**
- Create: `bsp/bsp_ZDT_Servo_emm_v5.c`
- Modify: `tests/stubs/ti_msp_dl_config.h`

- [ ] **Step 1: Port all seven frame builders.**

Copy protocol behavior from the supplied STM32 source while replacing its include path with `bsp_ZDT_Servo_emm_v5.h`. Preserve big-endian multi-byte encoding, signed QPos two's-complement encoding and exact frame lengths.

- [ ] **Step 2: Add blocking STEP1_UART transmission and compatibility calls.**

Use only generated names and DriverLib:

```c
static BSP_ZDT_Servo_Status transmitFrame(const uint8_t *frame, size_t length)
{
    if ((frame == NULL) || (length == 0U)) {
        return BSP_ZDT_SERVO_INVALID_ARGUMENT;
    }
    for (size_t i = 0U; i < length; ++i) {
        DL_UART_Main_transmitDataBlocking(STEP1_UART_INST, frame[i]);
    }
    return BSP_ZDT_SERVO_OK;
}
```

Each `BSP_EmmV5_*` send function builds into a fixed local buffer, snapshots the exact frame for diagnostics, and calls `transmitFrame`. Add:

```c
size_t BSP_ZDT_Servo_getLastTxFrame(uint8_t *buffer, size_t capacity);
```

- [ ] **Step 3: Run builder tests and verify GREEN.**

Expected output:

```text
zdt servo protocol tests passed
```

## Task 3: Add Standalone Command Parser

**Files:**
- Create: `tests/zdt_servo_command_test.c`
- Create: `app/ZDT_Servo_test.h`
- Create: `app/ZDT_Servo_test.c`

- [ ] **Step 1: Define fixed safe test parameters.**

```c
#define ZDT_SERVO_TEST_ADDRESS   (0x01U)
#define ZDT_SERVO_TEST_SPEED_RPM (60U)
#define ZDT_SERVO_TEST_ACCEL     (10U)
#define ZDT_SERVO_TEST_LINE_SIZE (64U)

void ZDT_Servo_Test_init(void);
void ZDT_Servo_Test_poll(void);
```

- [ ] **Step 2: Write failing command tests.**

Stub `DEBUG_UART_tryReadByte`, report functions and `BSP_EmmV5_*` send calls. Feed LF-terminated lines and assert:

```text
ZDT FWD       -> VelControl(addr=1, dir=0, vel=60, acc=10, snF=false)
ZDT REV       -> VelControl(addr=1, dir=1, vel=60, acc=10, snF=false)
ZDT STOP      -> StopNow(addr=1, snF=false)
ZDT POS 3200  -> SetQPosParams(relative-current), QPosControl(+3200)
ZDT POS -3200 -> SetQPosParams(relative-current), QPosControl(-3200)
```

Assert that `ZDT POS`, `ZDT POS 12x`, `ZDT UNKNOWN`, an overflowing integer and a line longer than 63 bytes produce no STEP1 send.

- [ ] **Step 3: Implement strict line parsing and diagnostics.**

`ZDT_Servo_Test_poll()` consumes bytes using `DEBUG_UART_tryReadByte()`, ignores CR, finalizes on LF and maintains an overflow flag. Use a manual `int32_t` decimal parser so no `strtol`/errno runtime dependency is introduced. After every successful call, print:

```text
ZDT OK TX: 01 FE 98 00 6B
```

On invalid input print one short `ZDT ERROR ...` line. `ZDT_Servo_Test_init()` resets parser state, sends STOP once and prints the command menu.

- [ ] **Step 4: Run command tests and verify GREEN.**

Expected output:

```text
zdt servo command tests passed
```

## Task 4: Switch Main To Standalone ZDT Test

**Files:**
- Modify: `app/main.c`

- [ ] **Step 1: Replace the active application loop with the minimal test.**

```c
#include "ti_msp_dl_config.h"
#include "app/ZDT_Servo_test.h"
#include "bsp/debug_uart.h"

int main(void)
{
    SYSCFG_DL_init();
    DEBUG_UART_initRxInterrupt();
    ZDT_Servo_Test_init();

    while (1) {
        ZDT_Servo_Test_poll();
        __WFI();
    }
}
```

Do not define the old timer ISR in this test entry point. Do not initialize encoder, PI tuner, OLED, TB6612, IMU, timer or Bluetooth.

- [ ] **Step 2: Compile the exact target sources with TI Arm Clang.**

Run `tiarmclang -fsyntax-only` with the existing project/SDK include paths over `app/main.c`, `app/ZDT_Servo_test.c`, `bsp/bsp_ZDT_Servo_emm_v5.c` and `bsp/debug_uart.c`. Expected: no diagnostics.

## Task 5: Static Configuration And CCS Build Verification

**Files:**
- Verify only: `empty.syscfg`, `Debug/ti_msp_dl_config.h`, CCS output

- [ ] **Step 1: Run the MSPM0 SysConfig checker.**

```powershell
python 'C:\Users\20778\.agents\skills\mspm0-ccs\scripts\check_syscfg.py' `
  'D:\MCU_Development\TI\MSPM0\nuedc2026_Pinconfig'
```

Confirm `STEP1_UART=UART1`, PB4 TX, PB5 RX and 115200 baud. Do not edit generated `Debug/ti_msp_dl_config.*`.

- [ ] **Step 2: Run a clean CCS build.**

```powershell
& 'C:\ti\ccs2100\ccs\utils\bin\gmake.exe' -C Debug clean all
```

Expected: exit code 0 and `Debug/nuedc2026_Pinconfig.out` exists. If the generated CCS makefile does not list new sources, refresh/rebuild the CCS project before interpreting unresolved symbols as firmware errors.

- [ ] **Step 3: Inspect the final diff.**

Require no changes to `empty.syscfg`, `Debug/` generated sources, existing control algorithms or existing BSP drivers. Record that compilation verifies software only; motor response remains unverified until hardware testing.

## Task 6: Hardware Test Procedure

- [ ] **Step 1: Wire and power safely.**

Connect PB4 TX to driver RX, PB5 RX to driver TX and GND to GND. Keep the motor mechanically unloaded or secured. Start with the driver disabled or motor power off, power the MCU, open RESV1 at 115200 8N1, then enable the driver/motor supply.

- [ ] **Step 2: Exercise commands in a stop-bounded sequence.**

```text
ZDT STOP
ZDT FWD
ZDT STOP
ZDT REV
ZDT STOP
ZDT POS 3200
ZDT STOP
ZDT POS -3200
ZDT STOP
```

For every line, confirm the RESV1 `ZDT OK TX:` bytes match the expected protocol and confirm physical direction/position separately. If motion direction is opposite to the labels, change the test's direction mapping only after recording the observed result.

## Plan Self-Review

- The plan implements only one EMM_V5 motor and excludes all K230, gimbal, laser and dual-axis actuator code.
- All confirmed UART, address, speed, acceleration and position-mode values are explicit.
- Protocol and command behavior are tested before target integration.
- The standalone main cannot initialize or command TB6612 or the line-follow control chain.
- No step modifies `empty.syscfg` or generated Debug sources.
- Hardware execution is explicitly separated from source/build verification.
