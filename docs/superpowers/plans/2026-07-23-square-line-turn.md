# Square Line Turn Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Detect each counterclockwise square corner when `OUT3/OUT4/OUT5` remain black for two 10 ms samples, stop for 100 ms, turn left 90 degrees with ICM-20948 yaw, and resume line following without retriggering on the same corner.

**Architecture:** Add a hardware-independent application state machine that composes the existing line and angle controllers. `PI_TUNER_update10ms()` reads one yaw sample, passes it with the track mask to the state machine, and keeps the existing wheel-speed PI and TB6612 output path.

**Tech Stack:** C11 host tests, TI Arm Clang, MSPM0 DriverLib/SysConfig, existing ICM-20948 and control modules.

---

### Task 1: State-machine tests

**Files:**
- Create: `tests/square_navigation_host_test.c`
- Modify: `tests/line_follow_host_test.c`

- [ ] Add tests that call `SQUARE_NAV_update10ms()` twice with `0x1C` and expect the second update to command zero and enter `STOPPING`.
- [ ] Advance nine more 10 ms updates and expect zero targets; on the following update expect `TURNING` with right `+220` and left `-220`.
- [ ] Supply yaw within five degrees of the captured yaw plus 90 degrees and expect zero targets plus `FOLLOW_WAIT_CLEAR`.
- [ ] Keep `0x1C` and verify it cannot retrigger, then supply `0x04` and verify the state re-arms.
- [ ] Cover wraparound, three consecutive invalid IMU updates, recovery from one invalid update, and the 3 s timeout.
- [ ] Run the test before production implementation and expect a compile failure because `square_navigation.h` does not exist.

### Task 2: Navigation implementation

**Files:**
- Create: `app/square_navigation.h`
- Create: `app/square_navigation.c`
- Modify: `control/line_follow.h`
- Modify: `control/line_follow.c`

- [ ] Define `SQUARE_NAV_State`, diagnostic output, 2-tick confirmation, 10-tick stopping, 3-failure IMU limit, 300-tick turn timeout, and a `+90.0f` relative target.
- [ ] Implement exact corner matching with `OUT3/OUT4/OUT5` black while ignoring `OUT1/OUT2`.
- [ ] Use `ANGLE_CONTROL_setTargetYawDeg(currentYawDeg + 90.0f)` after stopping and `ANGLE_CONTROL_update10ms()` during the turn.
- [ ] Treat `ANGLE_CONTROL_STATE_TURN_HOLD` as one-shot turn completion, disable angle control, reset only line dynamic state, and enter the clear-wait state.
- [ ] Run the navigation host test and expect all assertions to pass.

### Task 3: PI tuner integration

**Files:**
- Modify: `app/pi_tuner.c`
- Modify: `tests/pi_tuner_host_test.c`

- [ ] Extend the IMU test double with controllable yaw and read status.
- [ ] Add application assertions for corner stop, left-turn wheel targets, completed-turn line recovery, and `RUN=0` cancellation.
- [ ] Run the application test first and expect it to fail while `PI_TUNER` still calls the line controller directly.
- [ ] Cache the latest yaw in `PI_TUNER`, initialize the navigator, and replace direct line target generation with `SQUARE_NAV_update10ms()`.
- [ ] Reset the navigator whenever `RUN` is disabled or newly enabled; keep Bluetooth line and angle PID updates unchanged.
- [ ] Run the application test and expect it to pass.

### Task 4: Target checks

**Files:**
- Verify: `app/pi_tuner.c`, `app/square_navigation.c`, `control/line_follow.c`

- [ ] Run all affected MinGW host tests with `-Wall -Wextra -Werror`.
- [ ] Run TI Arm Clang `-fsyntax-only` over all affected application, control, and BSP sources.
- [ ] Run the `mspm0-ccs` SysConfig static checker and confirm no configuration change is needed.
- [ ] Report source/build-level results separately from real-car verification.

### Task 5: Turn indication

**Files:**
- Create: `bsp/turn_indicator.c`
- Create: `bsp/turn_indicator.h`
- Create: `tests/turn_indicator_host_test.c`
- Modify: `app/square_navigation.c`

- [ ] Write a failing host test for a 10-tick PB6 buzzer pulse, a 100-tick PA8 LED pulse, and immediate stop behavior.
- [ ] Implement nonblocking GPIO countdowns driven by `TURN_INDICATOR_update10ms()`.
- [ ] Start both outputs when corner confirmation enters `STOPPING`; stop both on initialization and fault.
- [ ] Run the indicator and navigation tests and expect all assertions to pass.
