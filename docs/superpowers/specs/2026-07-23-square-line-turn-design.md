# Square Line Turn Design

## Goal

Make the car follow a black square counterclockwise. When the advanced center
sensor and the two sensors to its left all detect the black corner, stop, turn
left by 90 degrees using the ICM-20948 yaw estimate, then return control to the
five-sensor line follower.

## Corner Pattern

`OUT1` is rightmost and `OUT5` is leftmost. Black is logic one. A corner
candidate therefore matches:

```c
(rawMask & (TRACK_MASK_OUT3 | TRACK_MASK_OUT4 | TRACK_MASK_OUT5)) ==
    (TRACK_MASK_OUT3 | TRACK_MASK_OUT4 | TRACK_MASK_OUT5)
```

`OUT1` and `OUT2` are deliberately ignored so small lateral errors do not hide
the corner. Two consecutive matching 10 ms updates trigger the turn sequence.

## State Machine

```text
FOLLOW_ARMED -> STOPPING -> TURNING -> FOLLOW_WAIT_CLEAR -> FOLLOW_ARMED
                                  \-> FAULT
```

- `FOLLOW_ARMED`: use the existing line controller and watch for the corner.
- `STOPPING`: command zero wheel speed for 100 ms.
- `TURNING`: capture the stopped yaw, command `yaw + 90 deg`, and use the
  existing angle controller until it enters `TURN_HOLD`.
- `FOLLOW_WAIT_CLEAR`: return immediately to line following but do not accept
  another corner until the corner pattern disappears.
- `FAULT`: command zero after repeated IMU failures or a turn timeout.

One invalid IMU update stops the wheels for that update. Three consecutive
invalid updates latch `FAULT`. A 3 s turn timeout also latches `FAULT`.

## Turn Indication

When the corner is confirmed and `STOPPING` begins, PB6 is driven high for
100 ms to sound `BUZZER_PIN`, and PA8 is driven high for 1 s to light
`LED_LED1`. The indicator uses the same 10 ms update and does not block motor
control. Initialization, `RUN=0`, and navigation fault paths clear both pins.

## Integration

Add `app/square_navigation.c/.h` to own only square-course sequencing. It
composes the existing `LINE_FOLLOW` and `ANGLE_CONTROL` modules. `PI_TUNER`
continues to own Bluetooth parameter updates, encoder speed updates, wheel PI,
and motor output polarity. It supplies the raw sensor mask and latest yaw to
the navigator every 10 ms.

The line follower gains and the current values of
`LINE_FOLLOW_CORRECTION_MAX_MMPS` and `LINE_FOLLOW_LOST_STOP_TICKS` are not
changed. A new line-dynamics reset clears stale integral/derivative state after
a turn without resetting live Bluetooth PID gains.

No SysConfig or generated `Debug/ti_msp_dl_config.*` file is modified.
`bsp/turn_indicator.c/.h` owns the active-high GPIO output timing.

## Verification

Host tests cover pattern confirmation, 100 ms stop time, left-turn targets,
90-degree completion, same-corner lockout, re-arming, yaw wraparound, transient
IMU failure, latched IMU fault, and turn timeout. Application tests verify the
PI tuner passes ICM yaw into the navigator and preserves `RUN=0` stopping.
