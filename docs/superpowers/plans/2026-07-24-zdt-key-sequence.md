# ZDT KEY1 Sequence Test Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Control the standalone ZDT motor test with debounced KEY1 presses and mirror every STEP1 binary command frame unchanged on RESV1.

**Architecture:** `bsp_ZDT_Servo_emm_v5.c` owns dual-UART byte transmission. `ZDT_Servo_test.c` owns the fixed action index and consumes stable key events from the existing BSP key scanner. `main.c` provides the required 10 ms polling cadence without enabling the old control timer.

**Tech Stack:** MSPM0G3507 DriverLib, SysConfig UART/GPIO symbols, TI Arm Clang, C11 host tests, CCS managed build.

---

### Task 1: Specify dual-UART frame transmission

**Files:**
- Modify: `tests/zdt_servo_protocol_test.c`
- Modify: `bsp/bsp_ZDT_Servo_emm_v5.c`

- [ ] Change the protocol test stub to record bytes separately for
  `STEP1_UART_INST` and `RESV1_UART_INST`, then require identical expected
  frames on both ports.
- [ ] Run the host test and confirm it fails because RESV1 receives no bytes.
- [ ] Update `transmitFrame()` to send each frame byte to STEP1 and RESV1.
- [ ] Re-run the protocol test and require `zdt servo protocol tests passed`.

### Task 2: Replace serial commands with the KEY1 sequence

**Files:**
- Modify: `tests/zdt_servo_command_test.c`
- Modify: `app/ZDT_Servo_test.h`
- Modify: `app/ZDT_Servo_test.c`

- [ ] Replace line-parser tests with stable-key event tests covering startup
  STOP, the eight post-start actions, release/hold behavior, ignored other
  keys and wraparound.
- [ ] Run the test and confirm it fails because the 10 ms key-update interface
  and sequence do not exist.
- [ ] Implement `ZDT_Servo_Test_update10ms()` using `KEY_scan10ms()` and a fixed
  action index. Keep the 1 ms delay between F1 and FC position frames.
- [ ] Remove RESV1 text output and receive parsing from the standalone test.
- [ ] Re-run the test and require `zdt servo key sequence tests passed`.

### Task 3: Integrate the 10 ms standalone loop

**Files:**
- Modify: `app/main.c`

- [ ] Initialize `KEY` after SysConfig and remove RESV1 RX interrupt setup from
  standalone mode.
- [ ] Call `delay_cycles(CPUCLK_FREQ / 100U)` followed by
  `ZDT_Servo_Test_update10ms()` forever.
- [ ] Confirm the inactive original smart-car branch remains unchanged.

### Task 4: Verify the target

**Files:**
- Verify: `empty.syscfg`
- Verify: `Debug/nuedc2026_Pinconfig.out`

- [ ] Run ZDT protocol, ZDT key-sequence and key-input host tests with warnings
  treated as errors.
- [ ] Run the MSPM0 SysConfig checker and confirm KEY1, STEP1_UART and RESV1_UART
  generated symbols remain present.
- [ ] Run a CCS full Debug build and require zero errors and zero warnings.
- [ ] Confirm `empty.syscfg` has no source diff and report that physical motor
  direction and motion still require board testing.

