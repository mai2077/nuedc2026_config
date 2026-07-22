# TB6612 Motor Polarity Test Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a small TB6612 DriverLib wrapper and a continuous, single-command motor polarity test through `app/main.c`.

**Architecture:** `bsp/tb6612.c` owns the TB6612 direction GPIO and PWM compare writes. `bsp/tb6612.h` exposes bounded signed-PWM APIs and test constants. `app/main.c` initializes SysConfig, calls the driver's safe initialization, and leaves four mutually exclusive test calls for manual comment/uncomment selection. `Debug/` generated files remain untouched.

**Tech Stack:** MSPM0G3507, TI DriverLib, SysConfig-generated `ti_msp_dl_config.h`, CCS managed build with TI Arm Clang.

---

### Task 1: Add the TB6612 driver interface

**Files:**
- Create: `bsp/tb6612.h`
- Create: `bsp/tb6612.c`

- [x] **Step 1: Define the public API and bounds**

Declare `TB6612_init`, `TB6612_setMotor1`, `TB6612_setMotor2`, and `TB6612_stopAll`; expose `TB6612_PWM_MAX` as 1000 and `TB6612_TEST_DUTY` as 150. Signed commands use positive for software-defined forward and negative for software-defined reverse.

- [x] **Step 2: Implement the direction and PWM mapping**

Include `ti_msp_dl_config.h` only in the C file. Use `TB_DIR_AIN1/2` and `TB_DIR_BIN1/2` for direction GPIOs, `TB_PWM_INST` with `DL_TIMER_CC_0_INDEX` for motor 1 and `DL_TIMER_CC_1_INDEX` for motor 2. Clamp commands to `[-1000, 1000]`, set one direction pin for nonzero commands, clear both direction pins for zero, and write the absolute command to the selected compare register.

- [x] **Step 3: Make initialization safe**

Call `TB6612_init()` after `SYSCFG_DL_init()` from the application. The driver itself must never enable a nonzero PWM during initialization; `TB6612_init()` delegates to `TB6612_stopAll()`, which writes zero to both PWM channels and clears all four direction pins.

### Task 2: Replace the application smoke test with a continuous polarity test

**Files:**
- Modify: `app/main.c`

- [x] **Step 1: Remove the PA4/PA5 test loop**

Remove the obsolete `gPA5InputLevel`, delay, LED toggle, and PA5 read code. Keep `SYSCFG_DL_init()` as the first hardware initialization call.

- [x] **Step 2: Add mutually exclusive continuous test calls**

After initialization and `TB6612_init()`, leave exactly four commented/uncommented examples:

```c
/* Enable exactly one line for the motor polarity test. */
/* TB6612_setMotor1(+TB6612_TEST_DUTY); */
/* TB6612_setMotor1(-TB6612_TEST_DUTY); */
/* TB6612_setMotor2(+TB6612_TEST_DUTY); */
/* TB6612_setMotor2(-TB6612_TEST_DUTY); */
```

Use an empty `while (1) {}` afterward so the selected PWM remains active continuously. Leave all four calls commented by default so a newly programmed board stays stopped until a test is deliberately selected.

### Task 3: Source-level verification without CCS build

**Files:**
- Inspect: `empty.c`, `Debug/ti_msp_dl_config.h`, `empty.syscfg`

- [x] **Step 1: Confirm there is only one `main` definition**

Search the project for `int main(` and confirm only `app/main.c` contains it. Do not restore or modify the removed `empty.c`.

- [x] **Step 2: Confirm generated names and pin mapping**

Check that the source references only generated symbols present in `Debug/ti_msp_dl_config.h`: `TB_PWM_INST`, `TB_DIR_AIN1_PIN`, `TB_DIR_AIN2_PIN`, `TB_DIR_BIN1_PIN`, `TB_DIR_BIN2_PIN`, and the two PWM channel indices.

- [x] **Step 3: Defer CCS compilation as requested**

Do not run `gmake`, CCS Clean/Build, flashing, or probe detection in this task. Report that compile and hardware validation remain pending.
