# ICM-20948 Gyroscope Diagnostic Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a polling ICM-20948 communication self-check and XYZ angular-velocity diagnostic that reports through `RESV1_UART` on a two-second start-to-start cycle.

**Architecture:** SysConfig owns I2C0 Controller setup on PA28/PA31. `bsp/icm20948.c` owns banked register access, repeated-START transactions, configuration verification, and gyro conversion; `bsp/debug_uart.c` owns numeric text output; `app/main.c` coordinates the two-second diagnostic using the existing 10 ms `TIMER_0` tick. The ISR only increments a counter.

**Tech Stack:** TI MSPM0G3507 DriverLib, SysConfig 1.26.2, MSPM0 SDK 2.10.00.04, TI Arm Clang 5.1.1.LTS, host GCC assertion tests.

---

This workspace is not a Git repository, so commit steps are intentionally replaced by source and test checkpoints.

### Task 1: Enable I2C0 Controller Mode

**Files:**
- Modify: `empty.syscfg`
- Regenerate: `Debug/ti_msp_dl_config.c`
- Verify: `Debug/ti_msp_dl_config.h`

- [x] **Step 1: Confirm the current failure condition**

Run:

```powershell
rg -n "basicEnableController|basicControllerBusSpeed" empty.syscfg
```

Expected before the change: no matches.

- [x] **Step 2: Add only the controller fields**

Insert next to the existing `ICM_I2C` name and pin assignments:

```javascript
I2C1.basicEnableController   = true;
I2C1.basicControllerBusSpeed = 100000;
```

Keep these assignments unchanged:

```javascript
I2C1.peripheral.$assign        = "I2C0";
I2C1.peripheral.sdaPin.$assign = "PA28";
I2C1.peripheral.sclPin.$assign = "PA31";
```

- [x] **Step 3: Regenerate SysConfig output**

Run:

```powershell
& 'C:\ti\sysconfig_1.26.2\sysconfig_cli.bat' -s 'C:\ti\mspm0_sdk_2_10_00_04\.metadata\product.json' --script 'empty.syscfg' -o 'Debug' --compiler ticlang
```

Expected: exit code 0 and regenerated output, not manual edits under `Debug/`.

- [x] **Step 4: Verify generated controller configuration and unchanged pins**

Run:

```powershell
rg -n "DL_I2C_resetControllerTransfer|Set frequency to 100000|DL_I2C_setTimerPeriod|DL_I2C_enableController|GPIO_ICM_I2C_(SDA|SCL)_PIN" Debug/ti_msp_dl_config.c Debug/ti_msp_dl_config.h
```

Expected: controller reset, 100 kHz timer configuration, controller enable, SDA pin 28, and SCL pin 31.

### Task 2: Add RESV1 UART Numeric Formatting

**Files:**
- Modify: `bsp/debug_uart.h`
- Modify: `bsp/debug_uart.c`
- Modify: `tests/track_uart_diag_test.c`

- [x] **Step 1: Write failing UART formatting assertions**

Add calls that require these outputs:

```c
resetTxBuffers();
DEBUG_UART_writeInt32(-2147483647L - 1L);
assert(strcmp(gDebugTxBuffer, "-2147483648") == 0);
assert(gBtTxLength == 0U);

resetTxBuffers();
DEBUG_UART_writeHexByte(0xEAU);
assert(strcmp(gDebugTxBuffer, "EA") == 0);

resetTxBuffers();
DEBUG_UART_writeSignedFixed3(-519);
assert(strcmp(gDebugTxBuffer, "-0.519") == 0);
```

- [x] **Step 2: Run the test and verify RED**

Run:

```powershell
gcc -DTRACK_DIAG_HOST_TEST -Itests/stubs -Ibsp tests/track_uart_diag_test.c bsp/track.c bsp/debug_uart.c bsp/bt_uart.c -o tests/track_uart_diag_test.exe
```

Expected: compile or link failure because the three new `DEBUG_UART_*` functions do not exist.

- [x] **Step 3: Add declarations and minimal implementations**

Add declarations:

```c
void DEBUG_UART_writeUInt32(uint32_t value);
void DEBUG_UART_writeInt32(int32_t value);
void DEBUG_UART_writeHexByte(uint8_t value);
void DEBUG_UART_writeSignedFixed3(int32_t milliValue);
```

Use division by ten for unsigned decimal output. Convert negative magnitude without overflowing `INT32_MIN`:

```c
uint32_t magnitude = (value < 0)
    ? ((uint32_t)(-(value + 1)) + 1U)
    : (uint32_t)value;
```

For fixed-three output, print the sign, integer part, decimal point, then exactly three fractional digits.

- [x] **Step 4: Rebuild and run the UART test**

Run the compile command from Step 2, then:

```powershell
.\tests\track_uart_diag_test.exe
```

Expected: `track/uart diagnostic tests passed` and no bytes in the BT buffer for `DEBUG_UART_*` calls.

### Task 3: Define the ICM Driver Contract and Failing Tests

**Files:**
- Create: `tests/icm20948_driver_test.c`
- Extend: `tests/stubs/ti_msp_dl_config.h`

- [x] **Step 1: Include and exercise the wished-for public API**

The test includes `icm20948.h` and must require:

```c
ICM20948_Status ICM20948_init(uint8_t *whoAmI);
ICM20948_Status ICM20948_selfCheck(uint8_t *whoAmI);
ICM20948_Status ICM20948_readGyro(ICM20948_GyroSample *sample);
int32_t ICM20948_rawToMilliDps(int16_t rawValue);
```

Status values must include OK, invalid argument, timeout, I2C error, WHO mismatch, and configuration mismatch.

- [x] **Step 2: Add a deterministic DriverLib I2C stub**

The stub provides `ICM_I2C_INST`, controller status constants, FIFO operations, advanced transfers, and `delay_cycles()`. It stores four 128-byte register banks and models:

```text
TX with STOP: write register data
TX without STOP: latch register address
RX after repeated START: expose sequential register bytes
```

Initialize Bank 0 `WHO_AM_I` to `0xEA`. Add switches that force controller error and timeout states.

- [x] **Step 3: Add behavior tests**

Assertions must cover:

```c
assert(ICM20948_rawToMilliDps(655) == 10000);
assert(ICM20948_rawToMilliDps(-655) == -10000);
assert(ICM20948_init(&who) == ICM20948_STATUS_OK);
assert(who == 0xEAU);
assert(bank0[0x06] == 0x01U);
assert(bank0[0x07] == 0x00U);
assert(bank2[0x00] == 0x0AU);
assert(bank2[0x01] == 0x1BU);
assert(ICM20948_selfCheck(&who) == ICM20948_STATUS_OK);
```

Load gyro bytes `00 0C FF DE 00 05` and require raw values `12`, `-34`, `5` with milli-dps `183`, `-519`, `76`. Change WHO to a non-`0xEA` value and require WHO mismatch. Change a config register and require configuration mismatch. Force busy/error states and require timeout/I2C error rather than an infinite loop.

- [x] **Step 4: Run the driver test and verify RED**

Run:

```powershell
gcc -DICM20948_HOST_TEST -Itests/stubs -Ibsp tests/icm20948_driver_test.c bsp/icm20948.c -o tests/icm20948_driver_test.exe
```

Expected: failure because `bsp/icm20948.c/.h` do not exist yet.

### Task 4: Implement Banked Register Access and Gyroscope Reading

**Files:**
- Create: `bsp/icm20948.c`
- Create: `bsp/icm20948.h`

- [x] **Step 1: Add fixed register and configuration constants**

Use:

```c
#define ICM20948_I2C_ADDRESS       (0x68U)
#define ICM20948_WHO_AM_I_VALUE    (0xEAU)
#define ICM20948_REG_BANK_SEL      (0x7FU)
#define ICM20948_REG_WHO_AM_I      (0x00U)
#define ICM20948_REG_PWR_MGMT_1    (0x06U)
#define ICM20948_REG_PWR_MGMT_2    (0x07U)
#define ICM20948_REG_GYRO_XOUT_H   (0x33U)
#define ICM20948_GYRO_DIV_VALUE    (0x0AU)
#define ICM20948_GYRO_CONFIG_VALUE (0x1BU)
#define ICM20948_TRANSFER_TIMEOUT  (100000UL)
```

- [x] **Step 2: Implement bounded transfer helpers**

Implement private helpers for idle wait, completion wait, register write, repeated-START register read, and bank selection. Before filling a TX FIFO, flush stale data. After each transfer start, call `delay_cycles(3U)` for I2C_ERR_13. Every polling loop decrements `ICM20948_TRANSFER_TIMEOUT` and returns timeout at zero; controller error returns `ICM20948_STATUS_I2C_ERROR`.

- [x] **Step 3: Implement initialization**

Perform the exact sequence:

```text
Bank 0 -> WHO=EA -> PWR_MGMT_1=01 -> PWR_MGMT_2=00
35 ms delay -> Bank 2 -> DIV=0A -> CONFIG=1B -> Bank 0
```

Use `delay_cycles(CPUCLK_FREQ * 35U / 1000U)` for gyro startup. Reject a null output pointer.

- [x] **Step 4: Implement repeated self-check**

Read and compare:

```text
Bank 0: WHO_AM_I=EA, PWR_MGMT_1=01, PWR_MGMT_2=00
Bank 2: GYRO_SMPLRT_DIV=0A, GYRO_CONFIG_1=1B
```

Always attempt to return to Bank 0 before returning. Return WHO mismatch separately from configuration mismatch.

- [x] **Step 5: Implement XYZ read and conversion**

Read six bytes from Bank 0 `0x33`, combine high then low into `int16_t`, and calculate:

```c
return ((int32_t)rawValue * 2000L) / 131L;
```

- [x] **Step 6: Run the driver test and verify GREEN**

Run the Task 3 compile command and executable. Expected: all driver assertions pass and the process exits 0.

### Task 5: Implement the Two-Second Diagnostic Application

**Files:**
- Modify: `app/main.c`

- [x] **Step 1: Add the 10 ms tick ISR**

Use the existing generated timer names:

```c
static volatile uint32_t gTick10ms;

void TIMER_0_INST_IRQHandler(void)
{
    switch (DL_TimerG_getPendingInterrupt(TIMER_0_INST)) {
        case DL_TIMER_IIDX_ZERO:
            ++gTick10ms;
            break;
        default:
            break;
    }
}
```

- [x] **Step 2: Add diagnostic print helpers in main**

Print one successful check line and one sample line using only `DEBUG_UART_*` functions:

```text
ICM CHECK ok who=0xEA pwr1=0x01 pwr2=0x00 div=0x0A cfg=0x1B
GYRO raw x=12 y=-34 z=5 dps x=0.183 y=-0.519 z=0.076
```

The self-check API verifies the configuration; the displayed expected configuration values are constants, while WHO is the value read in that cycle.

- [x] **Step 3: Replace the temporary track loop with the ICM loop**

Application order:

```c
SYSCFG_DL_init();
TB6612_init();
delay_cycles(CPUCLK_FREQ / 10U); /* 100 ms power-up */
NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
DL_TimerG_startCounter(TIMER_0_INST);
```

Initialize the ICM, then for each cycle record `cycleStart = gTick10ms`, run self-check, immediately read/print on success, print an error and mark the device uninitialized on failure, then wait while `(uint32_t)(gTick10ms - cycleStart) < 200U`. On a later cycle, an uninitialized device retries initialization before self-check.

- [x] **Step 4: Verify the application with TI Arm Clang**

Run:

```powershell
& 'C:\ti\ccs2100\ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\bin\tiarmclang.exe' -fsyntax-only -D__MSPM0G3507__ -D__USE_SYSCONFIG__ -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb -I. -IDebug -Ibsp -Iapp -Icontrol -I'C:\ti\mspm0_sdk_2_10_00_04\source\third_party\CMSIS\Core\Include' -I'C:\ti\mspm0_sdk_2_10_00_04\source' bsp\icm20948.c bsp\debug_uart.c app\main.c bsp\tb6612.c
```

Expected: exit code 0 with no diagnostics.

### Task 6: Run Final Static and Host Verification

**Files:**
- Verify: `empty.syscfg`
- Verify: `bsp/icm20948.c`
- Verify: `bsp/debug_uart.c`
- Verify: `app/main.c`
- Verify: `tests/track_uart_diag_test.c`
- Verify: `tests/icm20948_driver_test.c`

- [x] **Step 1: Run both host test executables from fresh builds**

Expected: zero assertion failures and exit code 0 for both.

- [x] **Step 2: Run the MSPM0 SysConfig static checker**

Run:

```powershell
python 'C:\Users\20778\.agents\skills\mspm0-ccs\scripts\check_syscfg.py' .
```

Expected: exit code 0, with no pin conflict or initialization-name errors.

- [x] **Step 3: Confirm generated ownership and UART separation**

Run:

```powershell
rg -n "DL_UART_Main_transmitDataBlocking\((RESV1_UART_INST|BT_UART_INST)|basicEnableController|basicControllerBusSpeed|PA28|PA31" bsp empty.syscfg
```

Expected: `DEBUG_UART_*` remains on RESV1, `BT_UART_*` remains on BT UART, and I2C pins are unchanged.

- [x] **Step 4: Remove temporary host executables**

Remove only `tests/track_uart_diag_test.exe` and `tests/icm20948_driver_test.exe`. Do not delete generated CCS outputs or user files.

- [x] **Step 5: Report hardware verification procedure**

State that software/static verification is complete but board behavior remains unverified. Describe the expected two-line cycle, stationary zero-rate tolerance, axis/sign response, and error output when the module is disconnected.
