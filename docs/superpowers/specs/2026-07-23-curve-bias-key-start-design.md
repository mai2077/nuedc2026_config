# Curve Bias Key Start Design

## Goal

Keep the vehicle stopped after power-up while accepting a curve feed-forward
bias from KEY3 and KEY4. Display the latest bias on the OLED, then latch it and
start line following when KEY1 is pressed.

## Key Input And Startup

The default bias is zero. While waiting, each KEY3 press adds 5 mm/s and each
KEY4 press subtracts 5 mm/s. The value is limited to plus or minus 500 mm/s.
Presses at a limit do not change the value or request an OLED refresh.

KEY1 changes the one-way startup state from `WAIT_BIAS` to `RUN`. The selected
bias and the compiled 500 mm/s base speed are latched. KEY3 and KEY4 are ignored
after startup. Bluetooth command reception and telemetry are disabled; RESV1
debug UART operation remains available.

## Line Controller

The bias is applied every 10 ms before final forward-only target clamping:

```text
right = base + curveBias - pidCorrection
left  = base - curveBias + pidCorrection
```

Positive bias assists a left curve; negative bias assists a right curve. A
bias change resets line PID dynamic memory. Straight, all-black, normal, and
short lost-line states all retain the same bias feed-forward.

## OLED

Before startup the OLED shows a static `WAIT KEY1` / `CURVE BIAS` page. A
revision counter causes only the signed bias field to refresh after each real
key adjustment. KEY1 starts the car without applying motor output in that same 10 ms
update, allowing `main.c` to replace the waiting page before the next control
update starts the motors.

## Safety And Scope

The accepted bias is clamped to plus or minus the compiled base speed. Wheel
targets retain their existing `0..WHEEL_SPEED_MAX_TARGET_MMPS` limit. KEY3
debug selection is replaced by bias increment while waiting. Square-corner
navigation, IMU turning, sound/light indication, and SysConfig remain unchanged.
