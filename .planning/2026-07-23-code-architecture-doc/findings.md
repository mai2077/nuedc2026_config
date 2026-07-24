# Findings

- The three-layer source tree contains BSP hardware adapters, reusable control
  algorithms, and application orchestration/diagnostic modules.
- `empty.syscfg` remains the hardware configuration source; `Debug/` is generated.
- Current runtime uses key-adjusted curve bias, direct line following, wheel PI,
  encoder feedback, TB6612 output, OLED display, and RESV1 debug UART.
- Square-turn, angle-control, and turn-indicator source is retained, but the
  current `PI_TUNER` path does not call square navigation.
- Bluetooth command reception and telemetry default to disabled.
- The active 10 ms timer ISR only snapshots encoder speed and increments
  `gTick10ms`; all control calculations remain in the main loop.
- `KEY1` starts the active run, `KEY3/KEY4` adjust curve bias by 5 mm/s before
  startup, and `KEY2` currently has no active-run stop action.
- The active line-follow path uses a 500 mm/s base target, black-high sensor
  bits, a correction limit of 150 mm/s, and stops after 15 all-white ticks.
- Wheel speed control is PI plus signed feedforward, with conditional integral
  update for saturation protection. Motor 1 output is negated for its physical
  mounting direction.
- RESV1 UART TX is blocking and RX uses a 128-byte overwrite-oldest ring;
  Bluetooth uses interrupt-driven RX/TX rings but is not initialized by main.
- IMU initialization is lazy and currently only requested by debug mode 2.
  The weak time hook returns zero unless overridden, so attitude integration
  falls back to a fixed 20 ms step.
