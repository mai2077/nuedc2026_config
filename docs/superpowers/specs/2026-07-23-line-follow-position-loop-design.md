# Line-Follow Position Loop Design

## Goal

Add a five-sensor line-following position outer loop to the existing two-wheel
speed PI control. While `RUN` is enabled, the line loop owns the right and left
wheel speed targets. The existing Bluetooth packet updates the line PID gains
during motion without changing the packet format.

## Existing Interfaces

- `TRACK_readRawMask()` returns raw GPIO levels in bits 4 through 0 as
  `OUT5` through `OUT1`.
- `OUT1` is the rightmost sensor and `OUT5` is the leftmost sensor.
- A raw level of one means black line detected; zero means white background.
- `WHEEL_SPEED_setTargetsMmps(right, left)` supplies targets to the existing
  10 ms wheel-speed PI inner loops.
- The Bluetooth binary packet already carries separate inner-speed, angle, and
  line PID values. Its `LINE PID` field starts at the existing line PID offset.
- `RUN=0` stops the vehicle. `RUN=1` enables line following.

## Architecture

The control flow is:

```text
five raw GPIO inputs
        |
        v
active-high masking and weighted line position
        |
        v
line position PID outer loop (10 ms)
        |
        v
right/left target speed difference
        |
        v
existing right/left speed PI inner loops (10 ms)
        |
        v
TB6612 motor outputs
```

The new `control/line_follow.c` and `control/line_follow.h` module owns line
position state, PID state, lost-line state, limits, and diagnostics. The BSP
continues to own only raw GPIO acquisition and bit packing.

## Sensor Model

The active black-line mask is:

```c
activeMask = (uint8_t)(rawMask & TRACK_MASK_ALL);
```

The sensor coordinates are:

```text
Sensor: OUT1  OUT2  OUT3  OUT4  OUT5
Side:   right              center             left
X mm:     +43   +15     0    -15   -43
Bit:        0     1     2      3     4
```

For one or more active sensors, position error is the arithmetic mean of the
active sensor coordinates. Positive error therefore means the line lies to the
right of the vehicle. Nonuniform coordinates are required because the outside
sensor spacing is larger than the center spacing.

## Position Controller

The controller runs every 10 ms and uses:

```text
integral += error * 0.01
derivative = (error - previous_error) / 0.01
correction = Kp * error + Ki * integral + Kd * derivative
```

Initial gains are:

```text
Kp = 3.0
Ki = 0.0
Kd = 0.05
```

The integral has symmetric anti-windup limiting. Changing any PID gain resets
the integral and derivative history so a live update cannot apply stale state
with the new gains.

The correction is clamped to both 150 mm/s and the magnitude of the current
forward base speed:

```text
correction_limit = min(150 mm/s, base_speed)
right_target = base_speed - correction
left_target  = base_speed + correction
```

Both targets are finally clamped to `0..WHEEL_SPEED_MAX_TARGET_MMPS`. The line
outer loop never commands wheel reversal. With the default 500 mm/s base speed,
the largest target pair is 350/650 mm/s or 650/350 mm/s.

## Special Sensor States

- Normal detection: compute the weighted error and update the PID.
- All white: retain the last valid error for up to two 10 ms updates. On the
  third consecutive all-white update, enter lost-line state and command both
  targets to zero.
- Reacquisition: any valid black detection clears lost-line state, resets the
  PID history, and resumes line following automatically.
- All black: treat the input as an intersection, use zero position error, and
  continue straight at the base speed.
- Symmetric multi-sensor patterns: their weighted mean is zero and therefore
  commands straight motion.

The lost-line timeout is based on consecutive control updates, not wall-clock
delays in the main loop.

## Bluetooth Integration

No SysConfig or Bluetooth packet layout change is required.

For each valid binary packet:

1. Decode all fields into temporary values.
2. Validate every PID value before changing controller state.
3. Apply `INNER PID` to both wheel-speed PI controllers.
4. Apply `LINE PID` through `LINE_FOLLOW_setPid()`.
5. Retain `ANGLE PID` and target yaw for future state-machine use, but do not
   let angle control generate targets while line-follow mode owns `RUN`.
6. Apply base speed and the `RUN` state.

Line gains must be finite and nonnegative. Limits are `Kp <= 50`, `Ki <= 20`,
and `Kd <= 5`. If any field is invalid, reject the complete packet so partial
parameter updates cannot occur.

The existing text command `SET P:... I:... D:...` continues to update the speed
PI only. It is not reinterpreted as a line command.

## Application Integration

`PI_TUNER_init()` initializes the line controller with the approved default
gains. `PI_TUNER_update10ms()` performs the following order while running:

1. Read the five raw track inputs.
2. Run the line outer loop to produce right and left speed targets.
3. Run the existing wheel-speed PI controllers using encoder measurements.
4. Apply the resulting PWM values to the TB6612 driver.

When stopped, both target speeds are zero and the TB6612 outputs are stopped.
The existing angle controller remains compiled and its interfaces remain
available, but it does not participate in line-follow target generation.

## Public API

The line controller provides stable interfaces similar to:

```c
void LINE_FOLLOW_init(void);
void LINE_FOLLOW_setBaseSpeedMmps(int16_t baseSpeedMmps);
uint8_t LINE_FOLLOW_setPid(float kp, float ki, float kd);
void LINE_FOLLOW_update10ms(uint8_t rawMask, LINE_FOLLOW_Output *output);
void LINE_FOLLOW_getOutput(LINE_FOLLOW_Output *output);
```

Diagnostics include raw mask, active mask, position error, PID terms,
correction, target speeds, lost-line count, and state.

## Testing

Host tests are written before production code and cover:

- active-high raw-to-active mask conversion;
- all five single-sensor weighted positions;
- nonuniform multi-sensor weighted positions;
- positive error slowing the right wheel and speeding the left wheel;
- negative error producing the inverse correction;
- 150 mm/s correction limiting;
- target speed limiting without reversal;
- all-white hold for two updates and stop on the third;
- automatic recovery after reacquisition;
- all-black straight behavior;
- live line PID update and controller-state reset;
- rejection of NaN, infinity, negative gains, and over-limit gains;
- Bluetooth packet `LINE PID` application without changing the speed PI or
  packet layout unexpectedly;
- `RUN=0` stopping both motors.

After host tests, target verification includes TI Arm Clang compilation,
SysConfig static checking, and a CCS Debug build. Hardware validation is kept
separate from source/build verification.

## Hardware Test Safety

The first firmware test must be performed with the drive wheels raised. Begin
with a low base speed and keep a direct motor-power disconnect available. Verify
sensor ordering and steering direction by moving a black strip from the center
toward `OUT1` and `OUT5` before allowing the vehicle to run on the floor.

## Out Of Scope

- IMU-assisted turns or heading fusion.
- Automatic switching between line following and fixed-yaw turning.
- Intersection route selection.
- Changes to the Bluetooth binary packet layout.
- Changes to `empty.syscfg` or generated files under `Debug/`.
