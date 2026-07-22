# ICM-20948 Minimal Driver Plan

## Goal

Implement a minimal ICM-20948 gyroscope diagnostic using `ICM_I2C` and send test output through `RESV1_UART`.

## Phases

- [complete] Inspect current SysConfig, datasheet registers, and local DriverLib APIs.
- [complete] Confirm the diagnostic design and any required SysConfig change with the user.
- [complete] Write and review the approved design specification.
- [complete] Create and self-review the implementation plan.
- [complete] Enable ICM_I2C Controller mode at 100 kHz and regenerate SysConfig output.
- [complete] Add and test RESV1 UART numeric formatting.
- [complete] Add host tests for the ICM register driver and observe the expected failure.
- [complete] Implement and pass tests for banked I2C access, initialization, self-check, and gyro conversion.
- [complete] Replace the temporary track main loop with the two-second ICM diagnostic.
- [complete] Run fresh host, SysConfig, UART-separation, and TI compiler verification.
- [complete] Write host tests for register parsing, scaling, and UART formatting.
- [complete] Implement `bsp/icm20948.c/.h` and the diagnostic application flow.
- [complete] Run host tests, SysConfig checks, and TI Arm Clang validation.

## 2026-07-19 IMU Example Compatibility Migration

- [complete] Compare the supplied I2C/IMU example interfaces with the current MSPM0 driver and identify missing dependencies.
- [complete] Confirm the missing binary-frame protocol/source with the user.
- [complete] Present migration approaches and a compatibility-focused design for approval.
- [complete] Write the approved design specification.
- [complete] Write the implementation plan after the user reviews the specification.
- [complete] Self-review the design specification.
- [complete] Obtain the user's written-spec review.
- [complete] Write and self-review the implementation plan.
- [complete] Add failing host tests for the retained example APIs and binary frame.
- [complete] Implement the TI I2C adapter, IMU compatibility layer, and 20 ms binary diagnostic.
- [complete] Run host tests, SysConfig checks, and TI Arm Clang validation.
- [complete] Add post-review I2C recovery, total timeout budgeting, FIFO bounds, startup scheduling, sample reinitialization, finite-value checks, and magnetometer fault isolation.
- [complete] Exclude host tests from CCS target builds and verify a clean Debug build after DriverLib redefinition errors.
- [complete] Implement the approved TB6612 KEY polarity-test design and verify host behavior, target syntax, SysConfig, and manual target link.
- [pending] Flash the current source and complete the physical UART/ICM/upper-computer checks.

## Constraints

- Do not edit generated files under `Debug/`.
- Keep existing `DEBUG_UART_*` and `BT_UART_*` interfaces intact.
- Do not claim hardware operation without a board test.
- Diagnostic self-check, read, and UART report must repeat on a 2-second full-cycle period.
- Preserve the existing `ICM20948_*` API unless an approved compatibility design explicitly supersedes it.
- Do not copy the example's unimplemented weak platform hooks into production without binding them to MSPM0 DriverLib.
- Use the STM32 `UserII2Dev` and `UserICM20948` behavior as the compatibility source; ignore the Renesas `SCI_IIC2_*` declarations.
- Encode upper-computer test output according to Anonymous Communication Protocol V8.10.
- Approved design document: `docs/superpowers/specs/2026-07-19-icm20948-ano-v8-design.md`.
- Approved implementation plan: `docs/superpowers/plans/2026-07-19-icm20948-ano-v8.md`.

## Errors Encountered

| Error | Attempt | Resolution |
| --- | --- | --- |
| PowerShell `rg` rejected `bsp/*.c` path syntax | 1 | Search the `bsp` directory directly without shell glob arguments. |
| System Python does not provide `pypdf` | 1 | Use the bundled document runtime or Poppler rendering instead of installing project dependencies. |
| PDF text output used the GBK console codec and rejected a symbol | 1 | Re-ran bundled Python with `-X utf8`; extraction succeeded. |
| Design workflow requested a Git commit, but this directory is not a Git repository | 1 | Keep the reviewed design document in the workspace and report that it cannot be committed here. |
| Temporary PDF render cleanup command was rejected because the automatic approval reviewer returned HTTP 503 | 1 | Do not retry or work around the rejection; leave the generated PNG files under `tmp/pdfs/` because they do not affect source or build output. |
| Scheduler wraparound test treated `UINT32_MAX-1` to `0` as one tick | 1 | Corrected the test to use `last=UINT32_MAX`; unsigned subtraction then gives one tick at `now=0` and two ticks at `now=1`. |
| TI Arm Clang rejected `NULL` in `bsp/ano_protocol.c` because the source lacked `<stddef.h>` | 1 | Added the standard header and reran the same syntax/type command. |
| Scheduler review found initialization could create a backlog of 10 ms sends after a long ICM startup delay | 1 | Added a failing late-execution test and `IMU_DIAG_reschedule` to align the next period to the current tick when overdue. |
| Legacy initialization reported WHO_AM_I mismatch before checking a failed I2C transaction | 1 | Added failing init error/timeout assertions and prioritized the diagnostic I2C status before WHO_AM_I comparison. |
