# ZDT KEY1 Sequence Test Design

## Goal

Replace the RESV1 text-command interface in the standalone ZDT EMM_V5 test
with one-button control. The motor starts in STOP and each debounced KEY1 press
advances one action through a fixed sequence.

## Sequence

Initialization sends STOP once for safety. Subsequent KEY1 presses execute:

```text
FWD -> STOP -> REV -> STOP -> POS 3200 -> STOP -> POS -3200 -> STOP
```

After the final STOP, the next KEY1 press returns to FWD. A held key must not
advance repeatedly. KEY2 through KEY4 have no effect.

## Key Handling

Reuse `KEY_init()` and `KEY_scan10ms()` from `bsp/key.c`. The standalone main
loop calls the ZDT test update every 10 ms. The existing two-sample debounce
therefore requires about 20 ms of stable input. Only a stable transition with
the KEY1 bit set advances the sequence; stable release merely arms the next
press.

## UART Behavior

Every binary frame sent to `STEP1_UART_INST` is also transmitted byte-for-byte
to `RESV1_UART_INST`, in the same byte order. RESV1 must not include text menus,
status messages, hexadecimal formatting, CR or LF because these bytes would
corrupt the raw frame stream.

The position actions each produce two mirrored frames: the F1 quick-position
parameter frame, followed by the FC signed relative-position frame.

## Reference Header Compatibility

The supplied STM32 header defines quick-position modes as `REL_LAST=0`,
`ABSOLUTE=1`, `REL_CURRENT=2`, and regular position modes as `RELATIVE=0`,
`ABSOLUTE=1`. The MSPM0 header must preserve these exact values and the original
`BSP_EMM_V5_DIR_*` / `BSP_EMM_V5_POS_*` macro names. Longer names from the
initial MSPM0 port remain compatibility aliases only.

## Main Program

The standalone main initializes SysConfig, initializes the key debouncer and
the ZDT test, then runs a 10 ms polling loop. It does not enable RESV1 receive
interrupts, encoder capture, TB6612 control, line following, OLED, IMU or the
control timer.

## Verification

- Host-test the exact KEY1 sequence, wraparound, release behavior and ignored
  non-KEY1 events.
- Host-test that STEP1 and RESV1 receive identical copies of every protocol
  frame.
- Run the existing key-input and ZDT protocol tests.
- Run SysConfig static validation and a complete CCS Debug build.
