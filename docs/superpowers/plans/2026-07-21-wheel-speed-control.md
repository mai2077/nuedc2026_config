# Wheel Speed Control Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add independently commanded, signed, closed-loop speed control for both wheels using the existing 10 ms encoder sample.

**Architecture:** A pure fixed-point PI module in `control/` converts mm/s targets to Q8 encoder counts and produces signed PWM. The application maps active-low keys to per-wheel targets and applies the right motor's physical inversion. The timer ISR remains short and only updates encoder speed plus the tick.

**Tech Stack:** MSPM0 DriverLib, SysConfig, TI Arm Clang, C11 host tests.

---

### Task 1: Correct right encoder polarity

**Files:**
- Modify: `bsp/board_config.h`
- Modify: `tests/encoder_host_test.c`

- [x] Change the host expectation so DIR1 low increments the right position and DIR1 high decrements it.
- [x] Run the encoder host test and observe failure with the current polarity macro.
- [x] Set `ENCODER1_DIR_HIGH_IS_POSITIVE` to `0U`.
- [x] Re-run the encoder host test and expect `encoder tests passed`.

### Task 2: Add the independent PI module

**Files:**
- Create: `control/wheel_speed.h`
- Create: `control/wheel_speed.c`
- Create: `tests/wheel_speed_host_test.c`

- [x] Write tests for 265 count/rev conversion, signed targets, independent controller state, zero-target reset, sign-change reset, PWM saturation and anti-windup.
- [x] Compile the test before implementation and verify failure because `wheel_speed.h/.c` do not exist.
- [x] Implement Q8 target conversion using the 150.796 mm wheel circumference.
- [x] Implement two PI states with initial `Kp=20`, `Ki=2`, output limit 700 and conditional integration.
- [x] Re-run and expect `wheel speed tests passed`.

### Task 3: Avoid PWM blanking on every control update

**Files:**
- Modify: `tests/tb6612_motor_test.c`
- Modify: `bsp/tb6612.c`

- [x] Add a failing test proving two same-direction duty updates do not invoke the direction-change delay twice.
- [x] Track the last direction per motor and blank only for an actual nonzero direction reversal.
- [x] Re-run and expect `tb6612 tests passed`.

### Task 4: Add the key-driven speed test application

**Files:**
- Create: `app/wheel_speed_test.h`
- Create: `app/wheel_speed_test.c`
- Create: `tests/wheel_speed_app_test.c`
- Modify: `app/main.c`

- [x] Test KEY1/KEY2 as right +/-400 mm/s, KEY3/KEY4 as left +/-400 mm/s, conflicts and releases as zero.
- [x] Implement the mapping and apply right PWM as `TB6612_setMotor1(-rightPwm)` and left PWM as `TB6612_setMotor2(leftPwm)`.
- [x] Replace direct polarity-test calls in `main.c` with target changes and one PI update per observed 10 ms tick.
- [x] Change OLED output to signed right/left target and measured mm/s.

### Task 5: Verify the target build

**Files:**
- Inspect only: `empty.syscfg`, `Debug/ti_msp_dl_config.c`, `Debug/ti_msp_dl_config.h`

- [x] Run all changed host tests and require zero failures.
- [x] Run the MSPM0 SysConfig static checker and confirm ENCODER1/2 and TIMER_0 remain unchanged.
- [ ] Run `gmake -C Debug clean all` and require a newly linked `nuedc2026_Pinconfig.out`.
- [x] Report that hardware polarity and PI tuning remain physical-board checks.
