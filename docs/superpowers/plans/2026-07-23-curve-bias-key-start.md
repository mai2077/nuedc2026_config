# Curve Bias Key Start Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Adjust and display a signed curve bias with KEY3/KEY4 while stopped, latch it with KEY1, and apply it as line-follow feed-forward after startup.

**Architecture:** Extend the hardware-independent line controller with a signed bias parameter. Keep key adjustment and KEY1 startup in `PI_TUNER`, and keep OLED rendering in `main.c` through read-only PI tuner getters.

**Tech Stack:** C11 host tests, TI Arm Clang, existing 10 ms scheduler, OLED software I2C, MSPM0 DriverLib.

---

### Task 1: Curve feed-forward

**Files:** `tests/line_follow_host_test.c`, `control/line_follow.c`, `control/line_follow.h`

- [x] Add failing tests for zero, positive, and negative bias plus final target clamping.
- [x] Add a signed bias setter and diagnostic field.
- [x] Apply `right += bias` and `left -= bias` to every line-follow target path.
- [x] Verify all line-follow tests pass.

### Task 2: Key adjustment and KEY1 start

**Files:** `tests/pi_tuner_host_test.c`, `app/pi_tuner.c`, `app/pi_tuner.h`

- [x] Add failing tests for default stopped state, 5 mm/s key steps, clamping, KEY1 start, and post-start bias locking.
- [x] Apply KEY3/KEY4 adjustments while waiting and expose bias/revision getters.
- [x] Start once on KEY1, keep the fixed compiled base speed, and return before motor control on the transition update.
- [x] Disable Bluetooth commands and telemetry while preserving RESV1 debug UART operation.
- [x] Verify PI tuner integration and square-navigation tests pass.

### Task 3: OLED waiting page

**Files:** `app/main.c`

- [x] Show `WAIT KEY1` and signed `CURVE BIAS` after OLED initialization.
- [x] Refresh only the signed bias field when a key adjustment changes its revision.
- [x] Replace the page with the existing speed/PID page after KEY1, before motor control begins on the following update.
- [x] Run TI Arm Clang source checking and SysConfig static checking.
