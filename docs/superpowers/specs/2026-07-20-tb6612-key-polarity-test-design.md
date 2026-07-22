# TB6612 Key-Controlled Motor Polarity Test Design

## Goal

Implement a safe, press-and-hold polarity test for the two TB6612 motor channels. Four active-low keys select motor 1 forward/reverse and motor 2 forward/reverse independently. Releasing a key stops its motor.

## Existing Hardware And Configuration

- MCU: MSPM0G3507.
- TB6612 `STBY` is tied to 3.3 V and is not software-controlled.
- `TB_PWM` uses TIMA1 at 32 MHz with period 1000, producing 32 kHz PWM.
- TIMA1 channel 0 uses PB0 for `PWMA` and controls motor 1.
- TIMA1 channel 1 uses PA16 for `PWMB` and controls motor 2.
- Motor 1 direction uses `AIN1` on PA2 and `AIN2` on PB21.
- Motor 2 direction uses `BIN1` on PB19 and `BIN2` on PB17.
- `KEY` contains four inputs on GPIOB: `KEY1` PB15, `KEY2` PB16, `KEY3` PB3, and `KEY4` PB8.
- Each key is externally pulled high and connects the input to ground when pressed. A 100 nF capacitor is connected between each key signal and ground.
- The generated key configuration correctly uses digital inputs with no internal resistor, so no SysConfig change is required.
- `TIMER_0` remains the existing 10 ms periodic TIMG7 timer and supplies the key-scan tick.

## PWM Semantics

`TB_PWM` uses `DL_TIMER_PWM_MODE_EDGE_ALIGN`, which counts down. The output becomes high at the load event and low at the compare-down event. Therefore, a smaller compare value produces a larger high-time duty cycle.

The public motor command remains a normalized signed value from `-1000` to `+1000`, where magnitude represents requested duty in per-mille:

```text
0       = 0% duty
150     = 15% duty
1000    = 100% duty
```

For the current down-counting PWM, the driver converts magnitude to a compare value using the timer load value:

```text
period = load + 1
duty_counts = round(period * magnitude / 1000)
compare = period - duty_counts
```

For the current `load=999`, command 150 produces `compare=850`. This replaces the current direct compare write, which incorrectly makes command 150 approximately 85% duty. A zero command is handled explicitly by writing the load value, not zero. The implementation reads the generated timer load value rather than duplicating the literal period in application code.

## Key Behavior

The stable pressed mask uses these bit assignments:

```text
bit 0: KEY1 -> motor 1 negative command (right wheel is physically reversed)
bit 1: KEY2 -> motor 1 positive command
bit 2: KEY3 -> motor 2 positive command
bit 3: KEY4 -> motor 2 negative command
```

The application applies these rules every time the debounced state changes:

| Keys | Motor command |
| --- | --- |
| KEY1 only within motor 1 pair | Motor 1 `-150` |
| KEY2 only within motor 1 pair | Motor 1 `+150` |
| KEY1 and KEY2 together | Motor 1 stop |
| Neither KEY1 nor KEY2 | Motor 1 stop |
| KEY3 only within motor 2 pair | Motor 2 `+150` |
| KEY4 only within motor 2 pair | Motor 2 `-150` |
| KEY3 and KEY4 together | Motor 2 stop |
| Neither KEY3 nor KEY4 | Motor 2 stop |

The two motor pairs are independent, so KEY1 and KEY3 may run both motors simultaneously. No command remains latched after release.

## Debouncing

The 100 nF capacitor suppresses short electrical edges, but its time constant depends on the external pull-up resistance. Firmware therefore retains deterministic software debouncing.

- Read all four active-low inputs once per 10 ms `TIMER_0` tick.
- Pack pressed keys into a four-bit mask.
- Accept a new stable mask only after two consecutive equal samples.
- Apply the new command immediately after the second equal sample.
- A release is debounced by the same rule, so the expected stop latency is 10-20 ms.

The timer ISR only increments a volatile tick. GPIO reads, debounce logic, and motor commands remain in the main loop.

## Source Layout

### `bsp/key.h` and `bsp/key.c`

Own the KEY GPIO interpretation and debounce state.

Public operations:

```c
void KEY_init(void);
uint8_t KEY_readPressedMask(void);
bool KEY_scan10ms(uint8_t *stableMask);
```

`KEY_readPressedMask` converts the active-low GPIO values into the four-bit pressed mask. `KEY_scan10ms` returns `true` only when a newly debounced stable mask is available.

### `bsp/tb6612.h` and `bsp/tb6612.c`

Retain the existing public operations:

```c
void TB6612_init(void);
void TB6612_setMotor1(int16_t signedPwm);
void TB6612_setMotor2(int16_t signedPwm);
void TB6612_stopAll(void);
```

Correct the signed-command-to-compare conversion for down-counting PWM. Clamp commands to `[-1000, +1000]`. A zero command clears both direction inputs and writes an actual 0% PWM compare value. Before applying a nonzero direction, remove drive, clear both direction pins, and wait one PWM period before selecting the new direction.

### `app/motor_polarity_test.h` and `app/motor_polarity_test.c`

Own the test mapping without reading GPIO directly.

Public operations:

```c
void MOTOR_POLARITY_TEST_init(void);
void MOTOR_POLARITY_TEST_applyKeys(uint8_t pressedMask);
```

The module translates a stable key mask into one signed command per motor. Conflicting keys stop only the affected motor.

### `app/main.c`

Temporarily becomes the motor polarity test entry point:

1. Initialize SysConfig.
2. Initialize keys and force both motors stopped.
3. Reset the 10 ms software tick and start `TIMER_0` interrupt handling.
4. On each new tick, run one key scan.
5. Apply motor commands only when the debounced mask changes.
6. Sleep with `__WFI()` between interrupts.

The ICM-20948 and Anonymous protocol modules remain in the project but are not initialized or called by this test application.

## Safety Behavior

- Both direction pairs and both PWM channels are placed in the stopped state immediately after `SYSCFG_DL_init()`.
- No key means both motors are stopped.
- Opposite-direction keys for one motor mean that motor is stopped.
- Commands are press-and-hold only; reset or key release cannot leave a latched run command.
- Test duty defaults to 15% and remains a named macro. If a motor cannot overcome static friction, it may be raised deliberately to 20-30% after checking wheel clearance and current.
- Since `STBY` is tied high, firmware cannot use standby as an emergency shutdown. Direction and PWM safe states are therefore mandatory.

## Testing And Verification

### Host Tests

- Verify command clamping and signed direction mapping.
- Verify 0%, 15%, and 100% commands produce the correct compare values for the generated load value.
- Verify zero command clears both direction pins.
- Verify active-low KEY1-KEY4 GPIO values map to bits 0-3.
- Verify two-sample debounce accepts presses and releases only after two equal 10 ms samples.
- Verify each key maps to the intended motor and sign.
- Verify KEY1+KEY2 and KEY3+KEY4 stop the affected motor.

### Target Verification

- Run the MSPM0 SysConfig static checker without editing generated files.
- Run all host tests with `-Wall -Wextra -Werror`.
- Run `gmake -C Debug clean all` and require `Debug/nuedc2026_Pinconfig.out`.
- Do not flash automatically.

### Hardware Procedure

1. Raise the chassis so both wheels can rotate freely.
2. Power the MCU first and confirm neither motor rotates with no key pressed.
3. Apply the 9 V motor supply.
4. Hold one key at a time and record the physical wheel direction.
5. Release each key and confirm the corresponding motor stops within the debounce interval.
6. Press both direction keys for one motor and confirm that motor remains stopped.
7. The current hardware observation defines the right-wheel (motor 1) software polarity: KEY1 is the negative command and KEY2 is the positive command. Keep this inversion in the application mapping without rewiring the PWM channels.
