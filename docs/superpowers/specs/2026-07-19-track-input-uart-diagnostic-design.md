# Track Input UART Diagnostic Design

## Goal

Read the five configured digital track-sensor inputs as one raw bit mask and print a human-readable diagnostic frame through `RESV1_UART` at 10 Hz. The diagnostic must expose electrical GPIO levels without assuming whether black or white is the active level.

## Verified Hardware Configuration

`RESV1_UART` is configured correctly in `empty.syscfg` and its generated DriverLib configuration:

- peripheral: UART0;
- TX: PA10 with UART0 TX pin function;
- RX: PA11 with UART0 RX pin function;
- clock: 32 MHz BUSCLK divided by 1;
- format: 115200 baud, 8 data bits, no parity, 1 stop bit;
- flow control: none;
- direction: TX and RX;
- operating mode: normal;
- generated UART initialization and enable are called by `SYSCFG_DL_init()`.

The five track inputs are configured as digital inputs on GPIOB:

| Diagnostic bit | Sensor | MCU pin | Physical order |
|---:|---|---|---|
| 0 | OUT1 | PB18 | rightmost |
| 1 | OUT2 | PB1 | right inner |
| 2 | OUT3 | PB10 | center |
| 3 | OUT4 | PB11 | left inner |
| 4 | OUT5 | PB14 | leftmost |

## Architecture

### `bsp/track.h` and `bsp/track.c`

The track BSP owns GPIO sampling and bit packing. It performs one `DL_GPIO_readPins(TRACK3_PORT, allPins)` operation so all five inputs are sampled from one GPIO input register read. It exposes `TRACK_readRawMask()`, returning only bits 0 through 4.

The BSP does not invert inputs, classify black/white, calculate line error, debounce signals, or implement lost-line logic. Those behaviors belong to later control and application layers after physical polarity is measured.

### `bsp/debug_uart.h` and `bsp/debug_uart.c`

The debug UART BSP owns blocking transmission through `RESV1_UART_INST`. It exposes byte and null-terminated string output helpers based on `DL_UART_Main_transmitDataBlocking()`. It does not use `printf`, interrupts, DMA, dynamic allocation, or receive handling.

### `app/main.c`

The application initializes SysConfig, calls `TB6612_init()` to keep both motors stopped, prints a one-time diagnostic header, and repeatedly:

1. reads the raw track mask;
2. emits one diagnostic frame;
3. delays approximately 100 ms.

No motor polarity-test call remains active during the track diagnostic.

## Diagnostic Protocol

The startup header is:

```text
TRACK DIAG bit4..bit0=OUT5..OUT1 raw GPIO levels\r\n
```

Each data frame is:

```text
TRACK mask=0x05 bits=00101\r\n
```

`mask` is always two hexadecimal digits in the range `0x00` through `0x1F`. The five binary digits are printed from bit 4 to bit 0, corresponding to `OUT5` through `OUT1`. At 115200 baud, the short blocking frame has negligible impact at the selected 10 Hz diagnostic rate.

## Safety And Error Handling

- Both motors are stopped immediately after `SYSCFG_DL_init()` through `TB6612_init()`.
- Track input polarity remains raw and explicit to avoid encoding an unverified black-line assumption.
- UART transmission is blocking; if UART0 cannot transmit, the diagnostic loop waits rather than silently dropping bytes.
- UART receive is configured but unused by this diagnostic.
- SysConfig-generated files under `Debug/` are inspection-only and are not edited.

## Verification

### Source And Build Verification

1. Run the MSPM0 SysConfig static checker.
2. Run the CCS Debug build and confirm `track.c`, `debug_uart.c`, and `app/main.c` compile and link.
3. Confirm there is exactly one `main()` definition.

### UART Verification

1. Connect PA10 TX to the USB-to-UART adapter RX and connect grounds.
2. Optionally connect adapter TX to PA11 RX; RX is not required for this test.
3. Open the serial terminal at 115200, 8-N-1, no flow control.
4. Reset the MCU and confirm the startup header appears once and data frames continue at approximately 10 frames per second.

### Track Input Verification

1. Record the idle mask with all sensors over the same surface.
2. Activate OUT1 alone and confirm only bit 0 changes.
3. Repeat through OUT5 and confirm bits 1, 2, 3, and 4 change respectively.
4. Activate two sensors together and confirm the hexadecimal mask equals the bitwise OR of their individual masks.
5. Check all-low and all-high states when the hardware can produce them: `0x00 / 00000` and `0x1F / 11111`.
6. Use the observed masks to determine whether black is represented by logic 1 or logic 0 before implementing line-following logic.

Hardware behavior is not considered verified until the UART waveform and all five input mappings are observed on the actual board.
