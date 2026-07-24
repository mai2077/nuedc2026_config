# ZDT Fixed-Angle Control Design

## Scope

Add a one-argument signed-degree interface for the existing single ZDT motor test. The accepted range is `-360..+360` degrees. Positive angles command counterclockwise rotation and negative angles command clockwise rotation.

## Conversion And Command Flow

The verified motor resolution is 3200 command pulses per revolution. Convert degrees to pulses with integer round-to-nearest arithmetic:

```text
pulses = round(angle_degrees * 3200 / 360)
```

The interface sends the existing quick-position parameter frame in `BSP_EMM_V5_QPOS_REL_CURRENT` mode, waits 1 ms, and sends the signed `FC` position frame. Inputs outside the accepted range return `BSP_ZDT_SERVO_INVALID_ARGUMENT` without transmitting. Zero degrees returns success without transmitting.

## Compatibility

Keep all existing pulse-level BSP interfaces. The KEY1 sequence retains its observed physical behavior by replacing `+3200/-3200` calls with `+360/-360` degree calls.

## Verification

Host tests cover `0`, `+1`, `-1`, `+45`, `-45`, `+90`, `-90`, `+360`, `-360`, and both out-of-range values. Existing protocol, key, SysConfig, and CCS Debug build checks must continue to pass.
