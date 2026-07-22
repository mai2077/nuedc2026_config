# Right-Wheel PI Auto-Tuner Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build an MSPM0G3507 test firmware that starts the right motor automatically, repeats a five-step speed profile, emits only numeric CSV telemetry, accepts silent `SET P:... I:... D:0` updates, and shows current values on the OLED.

**Architecture:** Keep the fixed-point 10 ms PI controller in `control/wheel_speed.*`; concentrate test-profile state, line-oriented UART parsing, and telemetry in `app/pi_tuner.*`; let `app/main.c` only initialize hardware and schedule UART, control, and display work. Preserve the existing RESV1_UART, encoder, TB6612, timer, and OLED BSP interfaces, and do not edit SysConfig-generated files.

**Tech Stack:** MSPM0G3507 DriverLib, SysConfig, TI Arm Clang 5.1.1 LTS, C11 host tests with MSYS2 GCC, fixed-point PI, RESV1_UART at its existing configuration, 128x64 OLED over existing software I2C.

---

### Task 1: Specify the new tuner behavior with failing host tests

**Files:**
- Modify: `tests/pi_tuner_host_test.c`
- Test: `tests/pi_tuner_host_test.c`

- [ ] **Step 1: Replace old key/left-wheel/command tests with automatic right-wheel tests.**

Keep the existing encoder, TB6612, and UART fakes, remove `key.h`, and add tests with these assertions:

```c
static void testStartsAutomaticallyAndOutputsOnlyNumericCsv(void)
{
    resetModel();
    assert(PI_TUNER_isRunning() != 0U);

    PI_TUNER_update10ms(1U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 300);
    assert(WHEEL_SPEED_getLeftTargetMmps() == 0);
    assert(gMotor1Command < 0);
    assert(gMotor2Command == 0);
    assert(strcmp(gTx,
        "10,300.00,0.00,105.00,300.00,20.000,0.000,0.000\r\n") == 0);
    assert(strchr(gTx, '#') == 0);
}

static void testProfileRepeatsEveryTenSeconds(void)
{
    resetModel();
    PI_TUNER_update10ms(1U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 300);
    PI_TUNER_update10ms(201U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 500);
    PI_TUNER_update10ms(401U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 700);
    PI_TUNER_update10ms(601U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 500);
    PI_TUNER_update10ms(801U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 300);
    PI_TUNER_update10ms(1001U);
    assert(WHEEL_SPEED_getRightTargetMmps() == 300);
    assert(PI_TUNER_isRunning() != 0U);
}
```

- [ ] **Step 2: Add tests for Arduino-style fixed-point commands and silent rejection.**

```c
static void testSetCommandUpdatesPiSilently(void)
{
    WHEEL_SPEED_Diagnostics diagnostics;
    resetModel();
    feedCommand("SET P:20.50 I:1.75 D:0\n", 10U);
    WHEEL_SPEED_getDiagnostics(WHEEL_SPEED_WHEEL_RIGHT, &diagnostics);
    assert(diagnostics.kpX100 == 2050U);
    assert(diagnostics.kiX100 == 175U);
    assert(gTx[0] == '\0');
}

static void testInvalidCommandsDoNotChangePiOrTransmitText(void)
{
    WHEEL_SPEED_Diagnostics before;
    WHEEL_SPEED_Diagnostics after;
    resetModel();
    WHEEL_SPEED_getDiagnostics(WHEEL_SPEED_WHEEL_RIGHT, &before);
    feedCommand("SET P:20 I:2 D:1\n", 10U);
    feedCommand("SET P:100.01 I:2 D:0\n", 10U);
    feedCommand("SET P:20 I:20.01 D:0\n", 10U);
    feedCommand("garbage\n", 10U);
    WHEEL_SPEED_getDiagnostics(WHEEL_SPEED_WHEEL_RIGHT, &after);
    assert(after.kpX100 == before.kpX100);
    assert(after.kiX100 == before.kiX100);
    assert(gTx[0] == '\0');
}
```

- [ ] **Step 3: Compile and run the test to verify RED.**

Run:

```powershell
& 'C:\msys64\ucrt64\bin\gcc.exe' -std=c11 -Wall -Wextra -Werror `
  -DPI_TUNER_HOST_TEST -Ibsp -Icontrol -Iapp `
  tests/pi_tuner_host_test.c app/pi_tuner.c control/wheel_speed.c `
  -o tests/pi_tuner_host_test_red.exe
& .\tests\pi_tuner_host_test_red.exe
```

Expected: compilation or assertion failure because the current API starts only on KEY1, prints protocol explanations, stops after one profile, and accepts only `PI,R,...`.

### Task 2: Implement automatic cycling and pure CSV telemetry

**Files:**
- Modify: `app/pi_tuner.h`
- Modify: `app/pi_tuner.c`
- Test: `tests/pi_tuner_host_test.c`

- [ ] **Step 1: Simplify the public tuner API.**

Use this interface and remove key and selected-wheel functions:

```c
void PI_TUNER_init(void);
void PI_TUNER_pollUart(void);
void PI_TUNER_update10ms(uint32_t tick10ms);
uint8_t PI_TUNER_isRunning(void);
```

- [ ] **Step 2: Replace the one-shot profile with a repeating right-wheel profile.**

Use five 200-tick steps and modulo indexing:

```c
#define PI_TUNER_STEP_TICKS       (200U)
#define PI_TUNER_PROFILE_TICKS    (1000U)

static const int16_t gProfileMmps[] = {300, 500, 700, 500, 300};

static int16_t piTunerProfileTarget(uint32_t tick10ms)
{
    uint32_t profileTick = tick10ms % PI_TUNER_PROFILE_TICKS;
    return gProfileMmps[profileTick / PI_TUNER_STEP_TICKS];
}
```

`PI_TUNER_init()` must initialize the wheel controller, stop both motor outputs, initialize the parser, and set `gRunning=1` without transmitting. `PI_TUNER_update10ms()` must always set `(rightTarget, leftTarget)=(profileTarget, 0)`, update both PI instances, command motor 1 with the existing physical polarity inversion, and command motor 2 with zero.

- [ ] **Step 3: Emit exactly one numeric CSV row every two control ticks.**

Use the existing UART formatting functions and no headers or status strings. Match the Arduino reference: setpoint/input/PWM/error use two decimal places and P/I/D are the current gains with three decimal places:

```c
DEBUG_UART_writeUInt32(tick10ms * 10U);
DEBUG_UART_writeByte(',');
piTunerWriteFixed2Integer(d.setpointMmps);
DEBUG_UART_writeByte(',');
piTunerWriteFixed2Integer(d.inputMmps);
DEBUG_UART_writeByte(',');
piTunerWriteFixed2Integer(d.outputPwm);
DEBUG_UART_writeByte(',');
piTunerWriteFixed2Integer(d.errorMmps);
DEBUG_UART_writeByte(',');
DEBUG_UART_writeSignedFixed3((int32_t)d.kpX100 * 10);
DEBUG_UART_writeByte(',');
DEBUG_UART_writeSignedFixed3((int32_t)d.kiX100 * 10);
DEBUG_UART_writeString(",0.000\r\n");
```

Set the initial last-telemetry tick so the first call at tick 1 emits a row, then emit only when two ticks have elapsed.

- [ ] **Step 4: Run the host test.**

Run the Task 1 command, changing the output to `tests/pi_tuner_host_test.exe`.

Expected at this intermediate checkpoint: automatic-profile and pure-CSV tests pass; SET command tests still fail.

### Task 3: Implement silent `SET P/I/D` parsing

**Files:**
- Modify: `app/pi_tuner.c`
- Test: `tests/pi_tuner_host_test.c`

- [ ] **Step 1: Add an overflow-safe decimal-to-x100 parser.**

The parser accepts unsigned `0`, `20`, `20.5`, or `20.50`, rejects signs, empty fields, extra characters, and more than two fractional digits:

```c
static uint8_t piTunerParseX100(const char *text, uint16_t *value)
{
    uint32_t whole = 0U;
    uint32_t fraction = 0U;
    uint8_t fractionDigits = 0U;
    uint8_t sawDigit = 0U;

    while ((*text >= '0') && (*text <= '9')) {
        sawDigit = 1U;
        whole = whole * 10U + (uint32_t)(*text++ - '0');
        if (whole > 655U) return 0U;
    }
    if (*text == '.') {
        ++text;
        while ((*text >= '0') && (*text <= '9')) {
            if (fractionDigits >= 2U) return 0U;
            fraction = fraction * 10U + (uint32_t)(*text++ - '0');
            ++fractionDigits;
        }
        if (fractionDigits == 0U) return 0U;
    }
    if ((sawDigit == 0U) || (*text != '\0')) return 0U;
    if (fractionDigits == 1U) fraction *= 10U;
    if ((whole * 100U + fraction) > 65535U) return 0U;
    *value = (uint16_t)(whole * 100U + fraction);
    return 1U;
}
```

- [ ] **Step 2: Parse exactly `SET P:<p> I:<i> D:<d>` and update only the right wheel.**

Split the completed line at spaces, require exactly four tokens (`SET`, `P:`, `I:`, `D:`), parse all three values with the x100 helper, require `dX100 == 0`, and call:

```c
if (WHEEL_SPEED_setGainsX100(
        WHEEL_SPEED_WHEEL_RIGHT, kpX100, kiX100) != 0U) {
    /* Applied silently; WHEEL_SPEED_setGainsX100 resets PI state. */
}
```

Malformed, overlong, out-of-range, and unknown lines must produce no UART output and no state change. `PI_TUNER_pollUart()` remains non-blocking and ignores CR before LF.

- [ ] **Step 3: Run the tuner test to verify GREEN.**

Run:

```powershell
& 'C:\msys64\ucrt64\bin\gcc.exe' -std=c11 -Wall -Wextra -Werror `
  -DPI_TUNER_HOST_TEST -Ibsp -Icontrol -Iapp `
  tests/pi_tuner_host_test.c app/pi_tuner.c control/wheel_speed.c `
  -o tests/pi_tuner_host_test.exe
& .\tests\pi_tuner_host_test.exe
```

Expected output: `pi tuner tests passed`.

### Task 4: Integrate automatic operation and OLED parameter display

**Files:**
- Modify: `app/main.c`
- Modify: `bsp/oled.h`
- Modify: `bsp/oled.c`
- Modify: `tests/oled_host_test.c`

- [ ] **Step 1: Add failing OLED tests for unsigned x100 display.**

Add `OLED_ShowFixed2(line, column, valueX100, integerDigits)` coverage that verifies `2050` renders `020.50` with `integerDigits=3`, and `175` renders `001.75`.

- [ ] **Step 2: Run the OLED test to verify RED.**

```powershell
& 'C:\msys64\ucrt64\bin\gcc.exe' -std=c11 -Wall -Wextra -Werror `
  -DOLED_HOST_TEST -Itests/stubs -Ibsp `
  tests/oled_host_test.c bsp/oled.c bsp/oled_soft_i2c.c `
  -o tests/oled_host_test_red.exe
& .\tests\oled_host_test_red.exe
```

Expected: compile failure because `OLED_ShowFixed2` does not yet exist.

- [ ] **Step 3: Implement `OLED_ShowFixed2`.**

Declare in `bsp/oled.h` and render a zero-padded integer portion, decimal point, and two fractional digits in `bsp/oled.c`:

```c
void OLED_ShowFixed2(uint8_t line, uint8_t column,
    uint16_t valueX100, uint8_t integerDigits)
{
    OLED_ShowNum(line, column, valueX100 / 100U, integerDigits);
    OLED_ShowChar(line, (uint8_t)(column + integerDigits), '.');
    OLED_ShowNum(line, (uint8_t)(column + integerDigits + 1U),
        valueX100 % 100U, 2U);
}
```

- [ ] **Step 4: Remove key startup logic and display right-wheel diagnostics.**

In `app/main.c`, remove `key.h`, `KEY_init`, `KEY_scan10ms`, and `PI_TUNER_applyKeys`. Initialize encoder before tuner, start the timer, and call `PI_TUNER_pollUart()` continuously. On each new 10 ms tick call `PI_TUNER_update10ms(now)`.

Snapshot the right-wheel diagnostics every 50 ticks (500 ms). Because software-I2C OLED writes are blocking, batch the eight glyph bytes per half-row and update at most one dynamic character on each non-telemetry tick. Use these fixed labels and values:

```c
OLED_ShowString(1U, 1U, "SP:");
OLED_ShowSignedNum(1U, 4U, diagnostics.setpointMmps, 4U);
OLED_ShowString(1U, 10U, "V:");
OLED_ShowSignedNum(1U, 12U, diagnostics.inputMmps, 3U);
OLED_ShowString(2U, 1U, "PWM:");
OLED_ShowSignedNum(2U, 5U, diagnostics.outputPwm, 4U);
OLED_ShowString(3U, 1U, "P:");
OLED_ShowFixed2(3U, 3U, diagnostics.kpX100, 3U);
OLED_ShowString(4U, 1U, "I:");
OLED_ShowFixed2(4U, 3U, diagnostics.kiX100, 3U);
OLED_ShowString(4U, 10U, "D:0");
```

- [ ] **Step 5: Run OLED and tuner host tests.**

Expected output:

```text
oled host tests passed
pi tuner tests passed
```

### Task 5: Replace the tuning prompt with the approved concise content

**Files:**
- Modify: `PI大模型调参提示词.md`

- [ ] **Step 1: Replace the old workflow-heavy prompt.**

The prompt must contain only these three sections and the final output contract:

```text
调参偏好：优先快速稳定，超调尽量小于 10%；单轮 P 或 I 调整不超过 25%；当前是 PI，D 固定为 0。
不能接受：持续 PWM=±700、明显持续振荡、积分长期堆积、方向错误、数据不足时继续增大参数。
对象特性：右轮速度环，10 ms 控制、20 ms CSV；265 count/rev、48 mm 轮径，1 count/10ms 约 56.9 mm/s；PWM 限幅 ±700；目标按 300/500/700/500/300 mm/s 每档 2 s 循环。
根据随后提供的 CSV 判断下一组参数。回复最后一行必须是：SET P:<0~100> I:<0~20> D:0
```

- [ ] **Step 2: Check that obsolete commands and protocol explanations are absent.**

Run:

```powershell
rg -n "PI,R|WHEEL|RUN|STOP|STATUS|#BEGIN|#END" 'PI大模型调参提示词.md'
```

Expected: no matches.

### Task 6: Full source and build verification

**Files:**
- Inspect: `empty.syscfg`
- Inspect: `Debug/ti_msp_dl_config.h`
- Inspect: all modified source and test files

- [ ] **Step 1: Run all directly affected host tests fresh.**

Run:

```powershell
& 'C:\msys64\ucrt64\bin\gcc.exe' -std=c11 -Wall -Wextra -Werror -DPI_TUNER_HOST_TEST -Ibsp -Icontrol -Iapp tests/pi_tuner_host_test.c app/pi_tuner.c control/wheel_speed.c -o tests/pi_tuner_host_test.exe
& 'C:\msys64\ucrt64\bin\gcc.exe' -std=c11 -Wall -Wextra -Werror -DWHEEL_SPEED_HOST_TEST -Icontrol tests/wheel_speed_host_test.c control/wheel_speed.c -o tests/wheel_speed_host_test.exe
& 'C:\msys64\ucrt64\bin\gcc.exe' -std=c11 -Wall -Wextra -Werror -DOLED_HOST_TEST -Itests/stubs -Ibsp tests/oled_host_test.c bsp/oled.c bsp/oled_soft_i2c.c -o tests/oled_host_test.exe
& 'C:\msys64\ucrt64\bin\gcc.exe' -std=c11 -Wall -Wextra -Werror -DENCODER_HOST_TEST -Itests/stubs -Ibsp tests/encoder_host_test.c bsp/encoder.c -o tests/encoder_host_test.exe
& 'C:\msys64\ucrt64\bin\gcc.exe' -std=c11 -Wall -Wextra -Werror -DTB6612_HOST_TEST -Itests/stubs -Ibsp tests/tb6612_motor_test.c bsp/tb6612.c -o tests/tb6612_motor_test.exe
& .\tests\pi_tuner_host_test.exe
& .\tests\wheel_speed_host_test.exe
& .\tests\oled_host_test.exe
& .\tests\encoder_host_test.exe
& .\tests\tb6612_motor_test.exe
```

Expected: five test executables exit 0 with their corresponding `tests passed` messages.

- [ ] **Step 2: Run TI Arm Clang target syntax/type validation.**

```powershell
& 'C:\ti\ccs2100\ccs\tools\compiler\ti-cgt-armllvm_5.1.1.LTS\bin\tiarmclang.exe' `
  -fsyntax-only -D__MSPM0G3507__ -D__USE_SYSCONFIG__ `
  -march=thumbv6m -mcpu=cortex-m0plus -mfloat-abi=soft -mlittle-endian -mthumb `
  -I. -IDebug -Ibsp -Iapp -Icontrol `
  -I'C:\ti\mspm0_sdk_2_10_00_04\source\third_party\CMSIS\Core\Include' `
  -I'C:\ti\mspm0_sdk_2_10_00_04\source' `
  app/main.c app/pi_tuner.c control/wheel_speed.c bsp/debug_uart.c `
  bsp/encoder.c bsp/tb6612.c bsp/oled.c bsp/oled_soft_i2c.c
```

Expected: exit code 0 with no diagnostics.

- [ ] **Step 3: Run the MSPM0 SysConfig static checker.**

```powershell
python 'C:\Users\20778\.agents\skills\mspm0-ccs\scripts\check_syscfg.py' .
```

Expected: no new ERROR findings; no `.syscfg` or generated file modification is needed.

- [ ] **Step 4: Run a clean CCS Debug build.**

```powershell
gmake -C Debug clean all
```

Expected: exit code 0; build log compiles `app/main.c`, `app/pi_tuner.c`, `control/wheel_speed.c`, `bsp/oled.c`, and links `Debug/nuedc2026_Pinconfig.out`. If the CCS makefiles are stale, refresh the CCS project and regenerate managed build files rather than editing `Debug/*.mk` manually.

- [ ] **Step 5: Review the final diff-like file list and safety handoff.**

Confirm no changes under `Debug/ti_msp_dl_config.*`, no edits to `empty.syscfg`, no residual key calls in `app/main.c`, no nonnumeric UART transmission in `app/pi_tuner.c`, and no left-motor nonzero command. Report that real hardware validation still requires raised wheels and a physical power disconnect because this test firmware has no key emergency stop.

---

## Plan self-review

- The plan covers automatic startup, repeating 300/500/700/500/300 profile, right-wheel-only output, 10 ms control, 20 ms numeric-only CSV, silent Arduino-style SET parsing, D=0 enforcement, OLED diagnostics, concise prompt, host tests, TI syntax checking, SysConfig checking, and CCS build verification.
- Public function names and fixed-point `x100` units are consistent across tuner, controller, OLED, tests, and prompt.
- No SysConfig-generated files are modified, and no unverified pin or peripheral names are introduced.
- The workspace is not a Git repository, so the usual per-task commits are unavailable; verification checkpoints replace commit steps.
