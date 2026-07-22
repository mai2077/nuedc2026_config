# LLM PI Tuner Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Generate safe single-wheel PI step-test CSV telemetry over RESV1_UART and accept bounded runtime PI commands.

**Architecture:** Extend the pure wheel-speed controller with runtime gains and diagnostic terms. Add nonblocking UART RX to the existing RESV1 backend. Add an application state machine for profile timing, key handling, commands, motor output and 20 ms CSV telemetry.

**Tech Stack:** MSPM0 DriverLib, UART0 polling RX, 10 ms TIMG7 tick, fixed-point PI, C11 host tests.

---

### Task 1: Runtime gains and diagnostics

**Files:**
- Modify: `tests/wheel_speed_host_test.c`
- Modify: `control/wheel_speed.h`
- Modify: `control/wheel_speed.c`

- [ ] Add failing tests for independent `kp_x100/ki_x100`, range rejection, reset after parameter change and p/i/d diagnostics.
- [ ] Change the PI math to divide gain products by `256 * 100` while preserving default Kp=20.00 and Ki=2.00.
- [ ] Store setpoint, input, error, PWM, p, i and d=0 per wheel.
- [ ] Run the wheel-speed host test and require `wheel speed tests passed`.

### Task 2: RESV1_UART nonblocking RX

**Files:**
- Modify: `tests/track_uart_diag_test.c`
- Modify: `tests/stubs/ti_msp_dl_config.h`
- Modify: `bsp/debug_uart.h`
- Modify: `bsp/debug_uart.c`

- [ ] Add a failing test for empty FIFO and one received byte.
- [ ] Implement `DEBUG_UART_tryReadByte()` with `DL_UART_Main_isRXFIFOEmpty()` and `DL_UART_Main_receiveData()`.
- [ ] Re-run the UART host test.

### Task 3: PI tuner state machine and parser

**Files:**
- Create: `tests/pi_tuner_host_test.c`
- Create: `app/pi_tuner.h`
- Create: `app/pi_tuner.c`

- [ ] Add failing tests for KEY1 start, KEY2 stop, the complete profile, single-wheel motor output, command range checks, busy rejection and exact CSV header/columns.
- [ ] Implement the fixed 0/300/500/700/500/300 profile, 20 ms telemetry and safe STOP path.
- [ ] Implement line commands `WHEEL`, `PI`, `RUN`, `STOP` and `STATUS` with explicit acknowledgements.
- [ ] Re-run the tuner host test.

### Task 4: Main integration and model prompt

**Files:**
- Modify: `app/main.c`
- Create: `PI大模型调参提示词.md`

- [ ] Replace the previous four-key fixed-speed test with KEY1 start and KEY2 stop.
- [ ] Poll UART continuously and run the tuner once per 10 ms tick.
- [ ] Document the data units, evaluation metrics, conservative gain changes and exact output command.

### Task 5: Verification

- [ ] Run encoder, wheel-speed, tuner, UART, TB6612 and key regression tests.
- [ ] Run SysConfig static validation and confirm RESV1_UART remains TX_RX on PA10/PA11 at 115200.
- [ ] Compile all changed target sources with TI Arm Clang and link an output containing the new tuner symbols.
- [ ] Report the CCS Refresh requirement for newly added source files and the remaining hardware test.
