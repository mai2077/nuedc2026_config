# TB6612 Key-Controlled Motor Polarity Test Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a safe press-and-hold motor polarity test where active-low KEY1-KEY4 independently command the two TB6612 channels at an actual 15% PWM duty.

**Architecture:** `bsp/tb6612.c` owns signed motor commands and down-counting PWM conversion, `bsp/key.c` owns active-low input packing and two-sample debounce, and `app/motor_polarity_test.c` maps stable key masks to motor commands. `app/main.c` becomes a small 10 ms timer-driven test entry point while the ICM modules remain compiled but inactive.

**Tech Stack:** MSPM0G3507 DriverLib, SysConfig-generated `ti_msp_dl_config.*`, TI Arm Clang 5.1.1 LTS, C11 host tests with GCC, CCS GNU Make build.

---

## File Structure

- Modify `bsp/tb6612.h`: keep the signed command API and define normalized PWM/test-duty constants.
- Modify `bsp/tb6612.c`: convert per-mille duty to down-counting compare values and enforce break-before-make direction switching.
- Create `bsp/key.h` and `bsp/key.c`: define key-mask bits, active-low GPIO packing, and 20 ms stable-state debounce.
- Create `app/motor_polarity_test.h` and `app/motor_polarity_test.c`: map key pairs to signed motor commands.
- Modify `app/main.c`: replace the ICM diagnostic loop with the timer-driven polarity test.
- Modify `tests/stubs/ti_msp_dl_config.h`: add the minimal timer, motor GPIO, and key symbols needed by host tests.
- Create `tests/tb6612_motor_test.c`, `tests/key_input_test.c`, and `tests/motor_polarity_test.c`: isolate hardware conversion, key debounce, and application mapping tests.
- Modify `task_plan.md` and `progress.md`: record implementation and verification status.

## Task 1: Correct The TB6612 PWM Driver

**Files:**
- Modify: `tests/stubs/ti_msp_dl_config.h`
- Create: `tests/tb6612_motor_test.c`
- Modify: `bsp/tb6612.h`
- Modify: `bsp/tb6612.c`

- [x] **Step 1: Extend the host DriverLib stub for TB_PWM and direction GPIO.**

Add a timer register type, test instances, generated-name-compatible direction pins, timer indexes, and function declarations:

```c
typedef struct {
    uint32_t unused;
} GPTIMER_Regs;

extern GPIO_Regs gTestGPIOA;
extern GPTIMER_Regs gTestTIMA1;

#define TB_PWM_INST (&gTestTIMA1)
#define TB_PWM_INST_CLK_FREQ (32000000U)
#define DL_TIMER_CC_0_INDEX (0U)
#define DL_TIMER_CC_1_INDEX (1U)
typedef uint32_t DL_TIMER_CC_INDEX;

#define TB_DIR_AIN1_PORT (&gTestGPIOA)
#define TB_DIR_AIN1_PIN  (1U << 2)
#define TB_DIR_AIN2_PORT (&gTestGPIOB)
#define TB_DIR_AIN2_PIN  (1U << 21)
#define TB_DIR_BIN1_PORT (&gTestGPIOB)
#define TB_DIR_BIN1_PIN  (1U << 19)
#define TB_DIR_BIN2_PORT (&gTestGPIOB)
#define TB_DIR_BIN2_PIN  (1U << 17)

uint32_t DL_TimerA_getLoadValue(const GPTIMER_Regs *timer);
void DL_TimerA_setCaptureCompareValue(
    GPTIMER_Regs *timer, uint32_t value, DL_TIMER_CC_INDEX index);
void DL_GPIO_setPins(GPIO_Regs *gpio, uint32_t pins);
void DL_GPIO_clearPins(GPIO_Regs *gpio, uint32_t pins);
```

- [x] **Step 2: Write the failing motor-driver host test.**

Create `tests/tb6612_motor_test.c` under `#ifdef TB6612_MOTOR_HOST_TEST`. Provide fake GPIO/timer functions with `load=999`, then assert:

```c
TB6612_init();
assert(gCompare[0] == 999U);
assert(gCompare[1] == 999U);
assert((gGPIOA | gGPIOB) == 0U);

TB6612_setMotor1(150);
assert((gGPIOA & TB_DIR_AIN1_PIN) != 0U);
assert((gGPIOB & TB_DIR_AIN2_PIN) == 0U);
assert(gCompare[0] == 850U);
assert(gLastDelayCycles == 1000U);

TB6612_setMotor1(-150);
assert((gGPIOA & TB_DIR_AIN1_PIN) == 0U);
assert((gGPIOB & TB_DIR_AIN2_PIN) != 0U);
assert(gCompare[0] == 850U);

TB6612_setMotor2(2000);
assert((gGPIOB & TB_DIR_BIN1_PIN) != 0U);
assert(gCompare[1] == 0U);

TB6612_stopAll();
assert(gCompare[0] == 999U);
assert(gCompare[1] == 999U);
assert((gGPIOA | gGPIOB) == 0U);
```

- [x] **Step 3: Compile and run the test to verify RED.**

Run:

```powershell
& 'C:\msys64\ucrt64\bin\gcc.exe' -std=c11 -Wall -Wextra -Werror `
  -DTB6612_MOTOR_HOST_TEST -Itests/stubs -Ibsp `
  tests/tb6612_motor_test.c bsp/tb6612.c `
  -o tests/tb6612_motor_test.exe
& '.\tests\tb6612_motor_test.exe'
```

Expected: assertion failure because the existing driver writes command magnitude directly and writes zero for stop.

- [x] **Step 4: Implement down-counting PWM conversion and safe switching.**

Keep these constants in `bsp/tb6612.h`:

```c
#define TB6612_PWM_MAX   (1000)
#define TB6612_TEST_DUTY (150)
```

Implement conversion using the live timer load:

```c
static uint32_t TB6612_stopCompare(void)
{
    return DL_TimerA_getLoadValue(TB_PWM_INST);
}

static uint32_t TB6612_dutyToCompare(uint32_t magnitude)
{
    uint32_t load = TB6612_stopCompare();
    uint32_t period = load + 1U;
    uint32_t dutyCounts;

    if (magnitude == 0U) {
        return load;
    }
    dutyCounts = (magnitude * period + (TB6612_PWM_MAX / 2U)) /
                 TB6612_PWM_MAX;
    if (dutyCounts >= period) {
        return 0U;
    }
    return period - dutyCounts;
}
```

In `TB6612_setMotor`, first write the stop compare value and clear both direction inputs. For a nonzero command, call `delay_cycles(DL_TimerA_getLoadValue(TB_PWM_INST) + 1U)`, set exactly one direction input, then write the converted compare value. `TB6612_stopAll` clears all four direction pins and writes the stop compare value to both channels.

- [x] **Step 5: Run the motor-driver test and verify GREEN.**

Expected output:

```text
tb6612 motor tests passed
```

## Task 2: Add Active-Low Key Reading And Debounce

**Files:**
- Modify: `tests/stubs/ti_msp_dl_config.h`
- Create: `tests/key_input_test.c`
- Create: `bsp/key.h`
- Create: `bsp/key.c`

- [x] **Step 1: Add KEY generated symbols to the host stub.**

```c
#define KEY_PORT     (&gTestGPIOB)
#define KEY_KEY1_PIN (1U << 15)
#define KEY_KEY2_PIN (1U << 16)
#define KEY_KEY3_PIN (1U << 3)
#define KEY_KEY4_PIN (1U << 8)
```

Reuse the existing `DL_GPIO_readPins` declaration.

- [x] **Step 2: Write the failing key test.**

Create `tests/key_input_test.c` under `#ifdef KEY_INPUT_HOST_TEST`. Model the physical GPIO value as high when released and low when pressed. Assert each active-low input maps to its public mask:

```c
gRawPins = KEY_ALL_PINS;
assert(KEY_readPressedMask() == 0U);
gRawPins = KEY_ALL_PINS & ~KEY_KEY1_PIN;
assert(KEY_readPressedMask() == KEY_MASK_KEY1);
gRawPins = KEY_ALL_PINS & ~(KEY_KEY2_PIN | KEY_KEY4_PIN);
assert(KEY_readPressedMask() == (KEY_MASK_KEY2 | KEY_MASK_KEY4));
```

Then assert two-sample debounce behavior:

```c
KEY_init();
gRawPins = KEY_ALL_PINS & ~KEY_KEY1_PIN;
assert(!KEY_scan10ms(&stable));
assert(KEY_scan10ms(&stable));
assert(stable == KEY_MASK_KEY1);

gRawPins = KEY_ALL_PINS;
assert(!KEY_scan10ms(&stable));
assert(KEY_scan10ms(&stable));
assert(stable == 0U);
```

- [x] **Step 3: Compile and verify RED.**

```powershell
& 'C:\msys64\ucrt64\bin\gcc.exe' -std=c11 -Wall -Wextra -Werror `
  -DKEY_INPUT_HOST_TEST -Itests/stubs -Ibsp `
  tests/key_input_test.c bsp/key.c -o tests/key_input_test.exe
```

Expected: compilation fails because `key.h` and `key.c` do not exist.

- [x] **Step 4: Implement the key BSP.**

Declare in `bsp/key.h`:

```c
#define KEY_MASK_KEY1 (1U << 0)
#define KEY_MASK_KEY2 (1U << 1)
#define KEY_MASK_KEY3 (1U << 2)
#define KEY_MASK_KEY4 (1U << 3)

void KEY_init(void);
uint8_t KEY_readPressedMask(void);
bool KEY_scan10ms(uint8_t *stableMask);
```

`KEY_readPressedMask` reads all four GPIO pins once and sets a mask bit when the corresponding physical pin is low. `KEY_init` resets candidate, stable, and consecutive-count state to zero. `KEY_scan10ms` requires two equal raw samples before publishing a changed stable mask and returns false for a null output pointer.

- [x] **Step 5: Run the key test and verify GREEN.**

Expected output:

```text
key input tests passed
```

## Task 3: Add The Polarity-Test Key Mapping

**Files:**
- Create: `tests/motor_polarity_test.c`
- Create: `app/motor_polarity_test.h`
- Create: `app/motor_polarity_test.c`

- [x] **Step 1: Write the failing mapping test.**

Create `tests/motor_polarity_test.c` under `#ifdef MOTOR_POLARITY_HOST_TEST`. Stub `TB6612_setMotor1`, `TB6612_setMotor2`, and `TB6612_stopAll` to capture commands. Assert:

```c
MOTOR_POLARITY_TEST_init();
assert(gStopAllCalls == 1U);

MOTOR_POLARITY_TEST_applyKeys(KEY_MASK_KEY1);
assert(gMotor1 == -TB6612_TEST_DUTY);
assert(gMotor2 == 0);

MOTOR_POLARITY_TEST_applyKeys(KEY_MASK_KEY2 | KEY_MASK_KEY3);
assert(gMotor1 == TB6612_TEST_DUTY);
assert(gMotor2 == TB6612_TEST_DUTY);

MOTOR_POLARITY_TEST_applyKeys(KEY_MASK_KEY1 | KEY_MASK_KEY2 |
                              KEY_MASK_KEY3 | KEY_MASK_KEY4);
assert(gMotor1 == 0);
assert(gMotor2 == 0);
```

- [x] **Step 2: Compile and verify RED.**

```powershell
& 'C:\msys64\ucrt64\bin\gcc.exe' -std=c11 -Wall -Wextra -Werror `
  -DMOTOR_POLARITY_HOST_TEST -Ibsp -Iapp `
  tests/motor_polarity_test.c app/motor_polarity_test.c `
  -o tests/motor_polarity_test.exe
```

Expected: compilation fails because the application module does not exist.

- [x] **Step 3: Implement the mapping module.**

Declare:

```c
void MOTOR_POLARITY_TEST_init(void);
void MOTOR_POLARITY_TEST_applyKeys(uint8_t pressedMask);
```

For each key pair, use an exact-mask switch so neither or both keys produce zero. Motor 2 keeps KEY3 as `+TB6612_TEST_DUTY` and KEY4 as `-TB6612_TEST_DUTY`. The physically reversed right wheel is motor 1, so invert that pair after decoding: KEY1 becomes `-TB6612_TEST_DUTY` and KEY2 becomes `+TB6612_TEST_DUTY`. Always send one command to each motor when applying a stable mask.

- [x] **Step 4: Run the mapping test and verify GREEN.**

Expected output:

```text
motor polarity tests passed
```

## Task 4: Integrate The Test Application

**Files:**
- Modify: `app/main.c`

- [x] **Step 1: Replace the active ICM diagnostic loop.**

Keep the existing `volatile uint32_t gTick10ms` and timer ISR. Replace ICM/Anonymous includes and application state with:

```c
#include "key.h"
#include "motor_polarity_test.h"

int main(void)
{
    uint32_t lastKeyTick = 0U;
    uint8_t stableKeys;

    SYSCFG_DL_init();
    MOTOR_POLARITY_TEST_init();
    KEY_init();

    gTick10ms = 0U;
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);
    DL_TimerG_startCounter(TIMER_0_INST);

    while (1) {
        uint32_t now = gTick10ms;

        if (now != lastKeyTick) {
            lastKeyTick = now;
            if (KEY_scan10ms(&stableKeys)) {
                MOTOR_POLARITY_TEST_applyKeys(stableKeys);
            }
        }
        __WFI();
    }
}
```

Do not initialize ICM-20948 or send UART data in this test application. The ISR remains limited to incrementing `gTick10ms` on `DL_TIMER_IIDX_ZERO`.

- [x] **Step 2: Run TI Arm Clang syntax/type validation.**

```powershell
& 'C:\ti\ccs2100\ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\bin\tiarmclang.exe' `
  -fsyntax-only -D__MSPM0G3507__ -D__USE_SYSCONFIG__ `
  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb `
  -I. -IDebug -Ibsp -Iapp -Icontrol `
  -I'C:\ti\mspm0_sdk_2_10_00_04\source\third_party\CMSIS\Core\Include' `
  -I'C:\ti\mspm0_sdk_2_10_00_04\source' `
  bsp/tb6612.c bsp/key.c app/motor_polarity_test.c app/main.c
```

Expected: no diagnostics.

## Task 5: Full Regression And CCS Build

**Files:**
- Modify: `task_plan.md`
- Modify: `progress.md`
- Verify only: `empty.syscfg`, generated `Debug/ti_msp_dl_config.*`, target output

- [x] **Step 1: Run the three new host tests.**

Require:

```text
tb6612 motor tests passed
key input tests passed
motor polarity tests passed
```

- [x] **Step 2: Run the existing host regression tests.**

Compile and run Anonymous protocol, I2C, IMU, legacy ICM, scheduler, and track/UART tests with their existing host macros. All must pass.

- [x] **Step 3: Run the SysConfig static checker.**

```powershell
python 'C:\Users\20778\.agents\skills\mspm0-ccs\scripts\check_syscfg.py' `
  'D:\MCU_Development\TI\MSPM0\nuedc2026_Pinconfig'
```

Confirm `KEY1-KEY4` are input GPIO, `TB_PWM` remains TIMA1 at 32 kHz, and no generated file was edited manually.

- [ ] **Step 4: Run a clean CCS Debug build after CCS refreshes its source list.**

```powershell
& 'C:\ti\ccs2100\ccs\utils\bin\gmake.exe' -C Debug clean all
```

Expected: exit code 0 and `Debug/nuedc2026_Pinconfig.out` exists. The existing command-line makefile was generated before the two new source files and therefore reports unresolved symbols until CCS refreshes the project. The new objects were separately compiled and linked with the same TI linker command as an interim target verification. Report SysConfig informational messages separately from build success.

- [ ] **Step 5: Update project tracking.**

Record the completed files, RED/GREEN test evidence, build result, and the fact that hardware direction remains unverified until the user performs the raised-wheel test. Do not flash automatically.

## Plan Self-Review

- Every confirmed design requirement maps to Tasks 1-5: corrected down-counting duty and safe stop (Task 1), active-low two-sample debounce (Task 2), independent key mapping and conflict stop (Task 3), 10 ms press-and-hold application flow (Task 4), and complete source/build verification (Task 5).
- All new production behavior has a failing host test before implementation.
- Public names and mask types are consistent across BSP, application, tests, and `main.c`.
- No step modifies `empty.syscfg` or generated `Debug/ti_msp_dl_config.*` manually.
- The plan contains no placeholders; hardware polarity is deliberately recorded as a physical verification outcome rather than guessed in firmware.
