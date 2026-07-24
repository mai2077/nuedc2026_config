# ZDT Fixed-Angle Control Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a signed `-360..+360` degree command for the existing single ZDT motor test.

**Architecture:** Keep raw EMM_V5 frame construction in the BSP. Add the one-argument degree wrapper to the existing ZDT test application, where the address, speed, acceleration, 1 ms inter-frame delay, and current-position mode are already owned.

**Tech Stack:** MSPM0G3507 DriverLib, EMM_V5 UART protocol, C11 host tests, TI Arm Clang, CCS managed build.

---

### Task 1: Specify Degree Conversion And Validation

**Files:**
- Modify: `tests/zdt_servo_command_test.c`
- Modify: `app/ZDT_Servo_test.h`
- Modify: `app/ZDT_Servo_test.c`

- [x] Add host assertions for symmetric rounded conversion, the two range limits, zero, and rejected out-of-range inputs.
- [x] Compile the command test and confirm it fails because `ZDT_Servo_rotateDegrees()` does not exist.
- [x] Declare and implement `ZDT_Servo_rotateDegrees(int16_t angleDegrees)` using 3200 pulses per 360 degrees.
- [x] Return `BSP_ZDT_SERVO_INVALID_ARGUMENT` for values outside `-360..+360`, and return success without UART commands for zero.
- [x] Run the command test and require `zdt servo key sequence and angle tests passed`.

### Task 2: Use The Angle Interface In The KEY1 Sequence

**Files:**
- Modify: `app/ZDT_Servo_test.c`
- Test: `tests/zdt_servo_command_test.c`

- [x] Replace the `+3200` action with `ZDT_Servo_rotateDegrees(360)`.
- [x] Replace the `-3200` action with `ZDT_Servo_rotateDegrees(-360)`.
- [x] Verify the existing eight-action sequence and wraparound remain unchanged.

### Task 3: Verify The MSPM0 Target

**Files:**
- Verify: `empty.syscfg`
- Verify: `Debug/nuedc2026_Pinconfig.out`

- [x] Rebuild and run the ZDT protocol, ZDT command, and key-input host tests with warnings treated as errors.
- [x] Run the MSPM0 SysConfig static checker and confirm `empty.syscfg` remains unchanged.
- [x] Run `C:\ti\ccs2100\ccs\utils\bin\gmake.exe -C Debug clean all` and require a successful link.
- [x] Report that physical partial-angle accuracy still requires on-board measurement.
