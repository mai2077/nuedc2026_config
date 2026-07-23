# Line-Follow Position Loop Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an active-high five-sensor line-follow position PID outer loop that supplies targets to the existing wheel-speed PI loops and accepts live `LINE PID` updates from the existing Bluetooth binary packet.

**Architecture:** Keep GPIO sampling in `bsp/track.c`; add a hardware-independent `control/line_follow.c/.h` controller; connect it in `app/pi_tuner.c` as the sole target generator while `RUN=1`. Preserve the Bluetooth packet layout, speed PI text command, angle-control files, motor polarity mapping, and SysConfig.

**Tech Stack:** C11 host tests with MinGW GCC, TI Arm Clang/DriverLib target code, MSPM0 SysConfig, CCS generated build.

---

## File Map

- Create `control/line_follow.h`: constants, state/output types, and public controller API.
- Create `control/line_follow.c`: active-high masking, weighted position, PID, limits, lost-line behavior, and diagnostics.
- Create `tests/line_follow_host_test.c`: deterministic controller tests without DriverLib.
- Modify `tests/pi_tuner_host_test.c`: Bluetooth binary RX model, track input model, and application-level line-follow assertions.
- Modify `app/pi_tuner.c`: initialize and run line following, validate/apply `LINE PID`, and stop using angle control as the `RUN` target source.
- Keep `bsp/track.c/.h`, `app/main.c`, `control/wheel_speed.c/.h`, `empty.syscfg`, and generated `Debug/ti_msp_dl_config.*` behavior unchanged unless compilation exposes a required include-only correction.

### Task 1: Pure Line-Follow Controller

**Files:**
- Create: `tests/line_follow_host_test.c`
- Create: `control/line_follow.h`
- Create: `control/line_follow.c`

- [ ] **Step 1: Write the failing controller test**

Create a host test guarded from CCS target builds:

```c
#ifdef LINE_FOLLOW_HOST_TEST

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "line_follow.h"

static void assertNear(float actual, float expected, float tolerance)
{
    float difference = actual - expected;
    if (difference < 0.0f) {
        difference = -difference;
    }
    assert(difference <= tolerance);
}

static LINE_FOLLOW_Output update(uint8_t rawMask)
{
    LINE_FOLLOW_Output output;
    LINE_FOLLOW_update10ms(rawMask, &output);
    return output;
}

static void testActiveLowSingleSensorPositions(void)
{
    static const uint8_t rawMasks[] = {0x01U, 0x02U, 0x04U, 0x08U, 0x10U};
    static const float errors[] = {43.0f, 15.0f, 0.0f, -15.0f, -43.0f};
    uint8_t i;

    for (i = 0U; i < 5U; ++i) {
        LINE_FOLLOW_init();
        LINE_FOLLOW_setBaseSpeedMmps(500);
        assertNear(update(rawMasks[i]).errorMm, errors[i], 0.001f);
    }
}

static void testWeightedAndSteeringDirection(void)
{
    LINE_FOLLOW_Output output;

    LINE_FOLLOW_init();
    LINE_FOLLOW_setBaseSpeedMmps(500);
    assert(LINE_FOLLOW_setPid(3.0f, 0.0f, 0.0f) != 0U);
    output = update(0x03U); /* OUT1 + OUT2 black: (43 + 15) / 2 = 29 */
    assertNear(output.errorMm, 29.0f, 0.001f);
    assert(output.rightTargetMmps == 413);
    assert(output.leftTargetMmps == 587);

    output = update(0x18U); /* OUT4 + OUT5 black: -29 */
    assert(output.rightTargetMmps > output.leftTargetMmps);
}

static void testCorrectionAndForwardOnlyLimits(void)
{
    LINE_FOLLOW_Output output;

    LINE_FOLLOW_init();
    LINE_FOLLOW_setBaseSpeedMmps(500);
    assert(LINE_FOLLOW_setPid(50.0f, 0.0f, 0.0f) != 0U);
    output = update(0x01U);
    assertNear(output.correctionMmps, 150.0f, 0.001f);
    assert(output.rightTargetMmps == 350);
    assert(output.leftTargetMmps == 650);

    LINE_FOLLOW_setBaseSpeedMmps(100);
    output = update(0x01U);
    assert(output.rightTargetMmps == 0);
    assert(output.leftTargetMmps == 200);
}

static void testLostLineIntersectionAndRecovery(void)
{
    LINE_FOLLOW_Output output;

    LINE_FOLLOW_init();
    LINE_FOLLOW_setBaseSpeedMmps(500);
    output = update(0x01U);
    output = update(0x00U);
    assert(output.state == LINE_FOLLOW_STATE_HOLD_LAST);
    output = update(0x00U);
    assert(output.state == LINE_FOLLOW_STATE_HOLD_LAST);
    output = update(0x00U);
    assert(output.state == LINE_FOLLOW_STATE_LOST);
    assert(output.rightTargetMmps == 0);
    assert(output.leftTargetMmps == 0);

    output = update(0x04U);
    assert(output.state == LINE_FOLLOW_STATE_TRACKING);
    assert(output.rightTargetMmps == 500);
    assert(output.leftTargetMmps == 500);

    output = update(0x1FU);
    assert(output.state == LINE_FOLLOW_STATE_INTERSECTION);
    assertNear(output.errorMm, 0.0f, 0.001f);
}

static void testLiveGainValidationAndReset(void)
{
    LINE_FOLLOW_Output output;

    LINE_FOLLOW_init();
    LINE_FOLLOW_setBaseSpeedMmps(500);
    assert(LINE_FOLLOW_setPid(3.0f, 1.0f, 0.05f) != 0U);
    (void)update(0x01U);
    assert(LINE_FOLLOW_setPid(2.0f, 0.0f, 0.0f) != 0U);
    output = update(0x01U);
    assertNear(output.derivativeMmPerS, 0.0f, 0.001f);
    assertNear(output.integralMmS, 0.43f, 0.001f);

    assert(LINE_FOLLOW_setPid(-1.0f, 0.0f, 0.0f) == 0U);
    assert(LINE_FOLLOW_setPid(50.01f, 0.0f, 0.0f) == 0U);
    assert(LINE_FOLLOW_setPid(0.0f, 20.01f, 0.0f) == 0U);
    assert(LINE_FOLLOW_setPid(0.0f, 0.0f, 5.01f) == 0U);
    assert(LINE_FOLLOW_setPid(NAN, 0.0f, 0.0f) == 0U);
    assert(LINE_FOLLOW_setPid(INFINITY, 0.0f, 0.0f) == 0U);
}

int main(void)
{
    testActiveLowSingleSensorPositions();
    testWeightedAndSteeringDirection();
    testCorrectionAndForwardOnlyLimits();
    testLostLineIntersectionAndRecovery();
    testLiveGainValidationAndReset();
    puts("line follow tests passed");
    return 0;
}

#endif /* LINE_FOLLOW_HOST_TEST */
```

- [ ] **Step 2: Run RED verification**

Run:

```powershell
& 'C:\msys64\ucrt64\bin\gcc.exe' -std=c11 -Wall -Wextra -Werror `
  -DLINE_FOLLOW_HOST_TEST -Icontrol `
  tests/line_follow_host_test.c control/line_follow.c `
  -o tests/line_follow_host_test.exe
```

Expected: compilation fails because `line_follow.h` and `line_follow.c` do not exist.

- [ ] **Step 3: Define the controller API**

Create `control/line_follow.h` with these exact public definitions:

```c
#ifndef LINE_FOLLOW_H_
#define LINE_FOLLOW_H_

#include <stdint.h>

#define LINE_FOLLOW_DEFAULT_KP          (3.0f)
#define LINE_FOLLOW_DEFAULT_KI          (0.0f)
#define LINE_FOLLOW_DEFAULT_KD          (0.05f)
#define LINE_FOLLOW_KP_MAX              (50.0f)
#define LINE_FOLLOW_KI_MAX              (20.0f)
#define LINE_FOLLOW_KD_MAX              (5.0f)
#define LINE_FOLLOW_CORRECTION_MAX_MMPS (150)
#define LINE_FOLLOW_LOST_STOP_TICKS     (3U)

typedef enum {
    LINE_FOLLOW_STATE_TRACKING = 0,
    LINE_FOLLOW_STATE_INTERSECTION,
    LINE_FOLLOW_STATE_HOLD_LAST,
    LINE_FOLLOW_STATE_LOST
} LINE_FOLLOW_State;

typedef struct {
    uint8_t rawMask;
    uint8_t activeMask;
    uint8_t lostLineTicks;
    LINE_FOLLOW_State state;
    float kp;
    float ki;
    float kd;
    float errorMm;
    float integralMmS;
    float derivativeMmPerS;
    float proportionalMmps;
    float integralMmps;
    float derivativeMmps;
    float correctionMmps;
    int16_t rightTargetMmps;
    int16_t leftTargetMmps;
} LINE_FOLLOW_Output;

void LINE_FOLLOW_init(void);
void LINE_FOLLOW_setBaseSpeedMmps(int16_t baseSpeedMmps);
uint8_t LINE_FOLLOW_pidIsValid(float kp, float ki, float kd);
uint8_t LINE_FOLLOW_setPid(float kp, float ki, float kd);
void LINE_FOLLOW_update10ms(uint8_t rawMask, LINE_FOLLOW_Output *output);
void LINE_FOLLOW_getOutput(LINE_FOLLOW_Output *output);

#endif /* LINE_FOLLOW_H_ */
```

- [ ] **Step 4: Implement the minimum controller**

Create `control/line_follow.c` with one private context and these rules:

```c
static const int8_t gSensorPositionMm[5] = {43, 15, 0, -15, -43};

activeMask = (uint8_t)(rawMask & 0x1FU);
errorMm = (float)positionSum / (float)activeCount;
integral = clamp(integral + errorMm * 0.01f, -200.0f, 200.0f);
derivative = hasPrevious ? (errorMm - previousErrorMm) / 0.01f : 0.0f;
correction = kp * errorMm + ki * integral + kd * derivative;
correctionLimit = min(150.0f, (float)baseSpeedMmps);
rightTarget = clamp(round(baseSpeedMmps - correction), 0, 1500);
leftTarget = clamp(round(baseSpeedMmps + correction), 0, 1500);
```

Use `value == value` plus bounded comparisons for NaN/Inf rejection, so target
code does not depend on `isfinite()`. On a gain change or line reacquisition,
clear integral, previous error, and derivative history. For all white, reuse the
last valid error for ticks one and two; on tick three write zero targets without
running another PID step. For all black, reset PID memory and write base/base.

- [ ] **Step 5: Run GREEN verification**

Run the Step 2 command, then:

```powershell
& .\tests\line_follow_host_test.exe
```

Expected: `line follow tests passed` and exit code 0.

- [ ] **Step 6: Commit the controller when repository permissions allow**

```powershell
git add -- control/line_follow.c control/line_follow.h tests/line_follow_host_test.c
git commit -m "feat: add line follow position controller"
```

### Task 2: Bluetooth LINE PID and Atomic Packet Application

**Files:**
- Modify: `tests/pi_tuner_host_test.c`
- Modify: `app/pi_tuner.c`
- Modify: `app/pi_tuner.h` only if a public diagnostic accessor is required

- [ ] **Step 1: Repair the host model around the current PI tuner**

Extend `tests/pi_tuner_host_test.c` with separate Bluetooth RX storage and a
raw track input. Add stubs for dependencies now used by `app/pi_tuner.c`:

```c
static uint8_t gBtRx[128];
static size_t gBtRxLength;
static size_t gBtRxIndex;
static uint8_t gTrackRawMask = 0x04U;

uint8_t BT_UART_tryReadByte(uint8_t *data)
{
    if ((data == 0) || (gBtRxIndex >= gBtRxLength)) {
        return 0U;
    }
    *data = gBtRx[gBtRxIndex++];
    return 1U;
}

uint8_t TRACK_readRawMask(void)
{
    return gTrackRawMask;
}

bool KEY_scan10ms(uint8_t *stableMask)
{
    if (stableMask != 0) {
        *stableMask = 0U;
    }
    return false;
}

void KEY_init(void) {}
```

Provide no-op `DISPLAY_report*`, IMU, and Anonymous-protocol stubs with the
exact signatures from their headers. Provide all `BT_UART_write*` stubs used by
telemetry. Keep debug text RX tests and RESV1-to-Bluetooth byte-forwarding tests.

- [ ] **Step 2: Add the failing binary packet tests**

Add little-endian helpers that build the existing 48-byte packet:

```c
static void writeFloatLe(uint8_t *buffer, uint8_t offset, float value);
static void writeI32Le(uint8_t *buffer, uint8_t offset, int32_t value);
static void feedBtPacket(uint8_t run, int32_t speedMmps,
    float innerP, float innerI, float innerD,
    float angleP, float angleI, float angleD,
    float lineP, float lineI, float lineD, float targetYawDeg);
```

The builder writes head `0xA5`, run at byte 1, speed at byte 2, inner PID at
byte 6, angle PID at byte 18, line PID at byte 30, target yaw at byte 42,
checksum at byte 46, and tail `0x5A` at byte 47. Use the checksum sum over bytes
1 through 45, matching the accepted packet format.

Add assertions:

```c
resetModel();
feedBtPacket(1U, 400, 20.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 2.0f, 0.0f, 0.0f, 0.0f);
gTrackRawMask = 0x01U;
PI_TUNER_update10ms(1U);
assert(WHEEL_SPEED_getRightTargetMmps() == 314);
assert(WHEEL_SPEED_getLeftTargetMmps() == 486);

resetModel();
feedBtPacket(1U, 400, 10.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 51.0f, 0.0f, 0.0f, 0.0f);
/* Verify speed, inner gains, and line gains remain at their pre-packet values. */
```

Also reject line NaN/Inf/negative/out-of-range values and confirm `RUN=0`
immediately leaves both target speeds and both motor commands at zero.

- [ ] **Step 3: Run RED verification**

Compile the updated harness with all current controller sources and required
test stubs:

```powershell
& 'C:\msys64\ucrt64\bin\gcc.exe' -std=c11 -Wall -Wextra -Werror `
  -DPI_TUNER_HOST_TEST -DPI_TUNER_ENABLE_BT_TELEMETRY=1 `
  -I. -Ibsp -Icontrol -Iapp `
  tests/pi_tuner_host_test.c app/pi_tuner.c `
  control/wheel_speed.c control/angle_control.c control/line_follow.c `
  -o tests/pi_tuner_host_test.exe
& .\tests\pi_tuner_host_test.exe
```

Expected: the new target and line-gain assertions fail because the decoded
`LINE PID` is stored but not applied and angle control still owns targets.

- [ ] **Step 4: Validate the complete packet before mutation**

In `piTunerHandleBtPacket()`:

1. Decode inner, angle, and line values into locals.
2. Convert and validate inner speed gains as today, including `D == 0`.
3. Require all angle gains to be finite and nonnegative.
4. Call `LINE_FOLLOW_pidIsValid(linePid.p, linePid.i, linePid.d)`.
5. Return after `DISPLAY_reportCommandRejected()` if any check fails.
6. Only after all checks pass, apply inner PI, line PID, stored angle PID,
   base speed, target yaw, and run state.

Do not call `LINE_FOLLOW_setPid()` before the rest of the packet has passed
validation. This preserves all-or-nothing application.

- [ ] **Step 5: Apply live line gains and base speed**

Use these calls after validation:

```c
gOuterLinePid = linePid;
gTargetMmps = targetMmps;
LINE_FOLLOW_setBaseSpeedMmps(targetMmps);
(void)LINE_FOLLOW_setPid(linePid.p, linePid.i, linePid.d);
```

Retain `gOuterAnglePid`, `ANGLE_CONTROL_setPid()`, and `gTargetYawDeg` for
future mode switching, but do not activate angle target generation from a
valid `RUN=1` packet.

- [ ] **Step 6: Run GREEN verification**

Run the Step 3 compile and executable commands. Expected: all PI tuner host
tests pass with exit code 0.

- [ ] **Step 7: Commit the integration when repository permissions allow**

```powershell
git add -- app/pi_tuner.c app/pi_tuner.h tests/pi_tuner_host_test.c
git commit -m "feat: apply Bluetooth line PID atomically"
```

### Task 3: Make Line Following the RUN Target Generator

**Files:**
- Modify: `app/pi_tuner.c`
- Test: `tests/pi_tuner_host_test.c`

- [ ] **Step 1: Add failing application-flow tests**

Add tests for these exact sequences:

```text
RUN=1, speed=500, center raw=0x04 -> right=500, left=500
RUN=1, OUT1 raw=0x01 -> right target < left target
RUN=1, OUT5 raw=0x10 -> right target > left target
RUN=1, three raw=0x00 updates -> both targets and motor outputs zero
RUN=1, center after lost -> both targets return to base speed
RUN=0 at any sensor state -> both targets and motor outputs zero immediately
```

Run the Task 2 host command. Expected: at least the center and steering target
tests fail against the old angle-control/straight-target path.

- [ ] **Step 2: Initialize line following**

In `PI_TUNER_init()`:

```c
WHEEL_SPEED_init();
ANGLE_CONTROL_init();
LINE_FOLLOW_init();
LINE_FOLLOW_setBaseSpeedMmps(PI_TUNER_FIXED_TARGET_MMPS);
gOuterLinePid.p = LINE_FOLLOW_DEFAULT_KP;
gOuterLinePid.i = LINE_FOLLOW_DEFAULT_KI;
gOuterLinePid.d = LINE_FOLLOW_DEFAULT_KD;
```

Keep `ANGLE_CONTROL_init()` because its debug/diagnostic interfaces remain in
the application. Disable it before the first run update.

- [ ] **Step 3: Replace target generation only**

Replace `piTunerApplyControlTargets()` with a hardware-light path:

```c
static void piTunerApplyControlTargets(void)
{
    LINE_FOLLOW_Output output;

    LINE_FOLLOW_update10ms(TRACK_readRawMask(), &output);
    WHEEL_SPEED_setTargetsMmps(
        output.rightTargetMmps, output.leftTargetMmps);
}
```

Do not change encoder reads, speed PI update order, right motor physical
inversion, left motor mapping, or `TIMER_0` ISR behavior.

- [ ] **Step 4: Make stop behavior immediate and repeatable**

When a packet sets `RUN=0`, call:

```c
gRunning = 0U;
ANGLE_CONTROL_disable();
WHEEL_SPEED_setTargetsMmps(0, 0);
TB6612_stopAll();
```

When a packet changes `RUN` back to one, reset the line controller by
reinitializing it, restoring the current base speed and current line PID, then
allow the next 10 ms update to acquire the sensor state. This prevents stale
lost-line and derivative state after a stop.

- [ ] **Step 5: Run application GREEN verification**

Run both host test executables:

```powershell
& .\tests\line_follow_host_test.exe
& .\tests\pi_tuner_host_test.exe
```

Expected: both print their pass messages and exit 0.

- [ ] **Step 6: Commit the flow change when repository permissions allow**

```powershell
git add -- app/pi_tuner.c tests/pi_tuner_host_test.c
git commit -m "feat: drive wheel targets from line position loop"
```

### Task 4: Target and Project Verification

**Files:**
- Inspect: `empty.syscfg`
- Inspect generated names only: `Debug/ti_msp_dl_config.h`
- Do not edit generated `Debug/` files

- [ ] **Step 1: Run all directly affected host tests fresh**

Rebuild and run the Task 1 and Task 2 commands from scratch. Require zero
compiler warnings, both pass messages, and zero exit codes.

- [ ] **Step 2: Run TI Arm Clang syntax/type checking**

```powershell
& 'C:\ti\ccs2100\ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\bin\tiarmclang.exe' `
  -fsyntax-only -D__MSPM0G3507__ -D__USE_SYSCONFIG__ `
  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft `
  -mlittle-endian -mthumb `
  -I. -IDebug -Ibsp -Iapp -Icontrol `
  -I'C:\ti\mspm0_sdk_2_10_00_04\source\third_party\CMSIS\Core\Include' `
  -I'C:\ti\mspm0_sdk_2_10_00_04\source' `
  control/line_follow.c app/pi_tuner.c app/main.c `
  control/wheel_speed.c control/angle_control.c bsp/track.c
```

Expected: exit code 0 and no diagnostics.

- [ ] **Step 3: Check SysConfig without modifying it**

```powershell
python 'C:\Users\20778\.agents\skills\mspm0-ccs\scripts\check_syscfg.py' .
```

Confirm `TRACK3` remains five GPIO inputs, `TIMER_0` remains a 10 ms periodic
TIMG7 timer, `BT_UART` remains UART2 on PA21/PA22 at 115200, and no generated
file was manually edited.

- [ ] **Step 4: Run a fresh CCS Debug build**

```powershell
& 'C:\ti\ccs2100\ccs\utils\bin\gmake.exe' -C Debug clean all
```

Expected: exit code 0 and a newly linked
`Debug/nuedc2026_Pinconfig.out`. If generated makefiles do not include
`control/line_follow.c`, refresh/reopen the CCS project so CCS regenerates its
source list; do not edit `Debug/control/subdir_vars.mk` manually.

- [ ] **Step 5: Review the final diff**

```powershell
git diff -- control/line_follow.c control/line_follow.h `
  app/pi_tuner.c app/pi_tuner.h tests/line_follow_host_test.c `
  tests/pi_tuner_host_test.c empty.syscfg Debug/ti_msp_dl_config.c `
  Debug/ti_msp_dl_config.h
```

Require no changes to `empty.syscfg` or generated SysConfig files and no
unrelated source changes.

- [ ] **Step 6: Hardware handoff**

Report source, host-test, SysConfig, and build results separately from hardware
behavior. First bench test with wheels raised, low speed, and motor-power cutoff
available. Move black tape from center toward `OUT1` and confirm the right wheel
target decreases; move it toward `OUT5` and confirm the left wheel target
decreases. Only then test on the floor.
