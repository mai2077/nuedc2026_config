# ICM-20948 Gyroscope Diagnostic Design

## Scope

Implement a minimal polling driver for ICM-20948 gyroscope bring-up. The diagnostic reads XYZ angular velocity and sends human-readable output through `RESV1_UART`. Accelerometer, magnetometer, DMP, FIFO, calibration, and heading integration are outside this stage.

Existing track, motor, `DEBUG_UART_*`, and `BT_UART_*` interfaces remain available. The diagnostic temporarily becomes the behavior of `app/main.c`, and the motor driver is initialized in its stopped state.

## Fixed Hardware Configuration

- ICM-20948 `nCS` is high, selecting I2C mode.
- `AD0` is low, so the fixed 7-bit target address is `0x68`.
- `FSYNC` is low because frame synchronization is unused.
- MSPM0 I2C0 uses PA28 for SDA and PA31 for SCL.
- External pull-ups are required on SDA and SCL.
- Diagnostic output uses `RESV1_UART`, UART0 TX on PA10, at 115200 baud.

## SysConfig Change

Keep the existing I2C instance and pins, and add only:

```text
ICM_I2C controller mode: enabled
ICM_I2C controller bus speed: 100000 Hz
```

`empty.syscfg` remains the source of truth. Generated files under `Debug/` must not be edited manually.

## Driver Interface

Add `bsp/icm20948.c` and `bsp/icm20948.h`.

The public interface contains:

```c
typedef enum {
    ICM20948_STATUS_OK = 0,
    ICM20948_STATUS_INVALID_ARGUMENT,
    ICM20948_STATUS_TIMEOUT,
    ICM20948_STATUS_I2C_ERROR,
    ICM20948_STATUS_WHO_AM_I_MISMATCH,
    ICM20948_STATUS_CONFIG_MISMATCH
} ICM20948_Status;

typedef struct {
    int16_t rawX;
    int16_t rawY;
    int16_t rawZ;
    int32_t milliDpsX;
    int32_t milliDpsY;
    int32_t milliDpsZ;
} ICM20948_GyroSample;

ICM20948_Status ICM20948_init(uint8_t *whoAmI);
ICM20948_Status ICM20948_selfCheck(uint8_t *whoAmI);
ICM20948_Status ICM20948_readGyro(ICM20948_GyroSample *sample);
int32_t ICM20948_rawToMilliDps(int16_t rawValue);
```

No dynamic allocation or floating-point formatting is used.

## Initialization Sequence

1. Wait 100 ms after `SYSCFG_DL_init()` before the first register access. The datasheet specifies a typical 11 ms and maximum 100 ms startup time for register access after power-up.
2. Select User Bank 0 through register `0x7F`.
3. Read `WHO_AM_I` register `0x00` and require `0xEA`.
4. Write `PWR_MGMT_1` register `0x06` with `0x01` to clear sleep and select the recommended clock source.
5. Write `PWR_MGMT_2` register `0x07` with `0x00` to keep all gyroscope and accelerometer axes enabled.
6. Wait at least 35 ms for gyroscope startup.
7. Select User Bank 2.
8. Write `GYRO_SMPLRT_DIV` register `0x00` with `10`, giving approximately 102.3 Hz output.
9. Write `GYRO_CONFIG_1` register `0x01` with `0x1B`: +/-500 dps, DLPF enabled, approximately 51.2 Hz bandwidth.
10. Return to User Bank 0 before reading sensor output.

## I2C Transactions

Register writes send register address and data in one controller transaction.

Register reads follow the datasheet sequence:

```text
START + 0x68(W) + register address + repeated START + 0x68(R) + data + STOP
```

Use `DL_I2C_startControllerTransferAdvanced()` so the register-address phase does not generate STOP. Apply the TI I2C_ERR_13 delay after starting each controller transfer.

Every wait for idle, transfer completion, or RX data uses a finite loop timeout. NACK, controller error, and timeout return an `ICM20948_Status`; the driver must not wait forever when the module is disconnected.

## Gyroscope Data

Read six consecutive bytes from User Bank 0 register `GYRO_XOUT_H` at `0x33`:

```text
X high, X low, Y high, Y low, Z high, Z low
```

Each axis is decoded as signed, big-endian 16-bit two's-complement data. At +/-500 dps, sensitivity is 65.5 LSB/(degree/second), so integer conversion is:

```text
milli_dps = raw * 2000 / 131
```

The initial test does not subtract zero-rate offset. A stationary module may therefore report small non-zero values; the datasheet permits an initial zero-rate offset up to approximately +/-5 dps.

## UART Diagnostic

Extend `debug_uart.c/.h` with generic signed integer, hexadecimal-byte, and signed fixed-three-decimal output helpers. Existing debug functions remain unchanged and continue using `RESV1_UART_INST`.

On successful initialization, output once:

```text
ICM init addr=0x68
ICM OK who=0xEA fs=500dps odr=102Hz
```

Every two-second cycle reads `WHO_AM_I` and reads back `PWR_MGMT_1`, `PWR_MGMT_2`, `GYRO_SMPLRT_DIV`, and `GYRO_CONFIG_1`. A successful cycle outputs:

```text
ICM CHECK ok who=0xEA pwr1=0x01 pwr2=0x00 div=0x0A cfg=0x1B
```

The program then immediately reads one gyroscope sample and outputs:

```text
GYRO raw x=12 y=-34 z=5 dps x=0.183 y=-0.519 z=0.076
```

At rest, values should remain near zero but need not be exactly zero. Rotating the board around one axis should produce the largest magnitude on that axis, and reversing rotation should reverse the sign.

Initialization failures are printed on the cycle in which they occur:

```text
ICM init error=3
```

Self-check or read failures are reported without blocking future two-second cycles. A failed self-check skips the data read for that cycle:

```text
ICM CHECK error=5
ICM read error=2
```

## Application Flow

```text
SYSCFG_DL_init
  -> TB6612_init (motors stopped)
  -> wait for ICM power-up
  -> initialize ICM20948
  -> start existing TIMER_0 10 ms tick
  -> record cycle start tick
  -> run ICM20948_selfCheck
  -> if successful, immediately run ICM20948_readGyro
  -> print through DEBUG_UART_* / RESV1_UART
  -> wait until 200 ticks have elapsed from cycle start
  -> repeat
```

`TIMER_0` interrupt handling only increments a 10 ms counter. All I2C operations, checks, conversion, and UART output remain in the main loop. This makes the complete start-to-start cycle approximately 2 seconds, including self-check, sample read, and UART transmission.

## Verification

- Host tests verify byte decoding, negative two's-complement values, +/-500 dps conversion boundaries, configuration self-check results, UART fixed-point output, and preservation of separate RESV1/BT UART instances.
- SysConfig static validation confirms I2C0 Controller mode, 100 kHz, and unchanged PA28/PA31 pins.
- TI Arm Clang syntax/type validation covers the driver and application sources.
- Hardware validation confirms `WHO_AM_I=0xEA`, stationary readings, per-axis response, sign reversal, and recovery/error output when the module is disconnected.
