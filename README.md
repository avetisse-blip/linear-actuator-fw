# Linear actuator BLDC first bring-up

Small PlatformIO firmware for the first safe bench run of:

- NUCLEO-L476RG;
- SimpleFOC Mini v1.0 (DRV8313, 3PWM);
- 2804 12N14P gimbal BLDC (7 pole pairs);
- AS5600 over I2C.

The motor never starts after reset. The first USER-button press performs sensor
alignment and starts a slow velocity command. The next presses toggle stop/run;
alignment is performed only once after each reset.

## Safety limits for the first run

- Laboratory PSU: `12 V`, current limit `0.50 A`.
- Firmware motor and driver voltage limit: `1.0 V`.
- Test speed: `1.0 rad/s`.
- Maximum accepted command: `5.0 rad/s`.
- Velocity feedback low-pass time constant: `0.05 s`.

The motor is listed as only `5.1 ohm` line-to-line, while the SimpleFOC Mini
documentation recommends higher-resistance gimbal motors. Do not raise the
limits for the first test, and do not connect an unrestricted supply.

Remove all mechanical load and keep clear of the rotor. FOC alignment moves the
rotor when the USER button is pressed.

## Wiring

All grounds must be common.

| SimpleFOC Mini v1.0 | Nucleo header | STM32 pin |
|---|---|---|
| IN1 | D7 | PA8 / TIM1_CH1 |
| IN2 | D8 | PA9 / TIM1_CH2 |
| IN3 | D2 | PA10 / TIM1_CH3 |
| EN | D4 | PB5 |
| nFT / nFAULT | D5 | PB4 |
| nRT / nRESET | D6 | PB10 |
| nSP / nSLEEP | D9 | PC7 |
| GND | GND | GND |

Connect Mini `+/-` only to the laboratory PSU. Leave the Mini `3V3` output
unconnected. Connect the motor phases to `M1/M2/M3` in any fixed order.

| AS5600 P1 | Cable | Nucleo header | STM32 pin |
|---|---|---|---|
| VCC | red | 3V3 | - |
| GND | black | GND | - |
| SCL | yellow | D15 | PB8 / I2C1_SCL |
| SDA | green | D14 | PB9 / I2C1_SDA |

AS5600 P1 physical order is `VCC - GND - SCL - SDA`; P2 is unused. The onboard
blue USER button B1 is connected to PC13.

`nFT`, `nRT`, and `nSP` are active-low. Firmware configures `nFT` as an input
with pull-up, and drives `nRT` and `nSP` high for normal operation. D0/PA3 and
D1/PA2 are not used as driver GPIO and remain reserved for USART2.

The more detailed wiring checklist is in
[`../WIRING_FIRST_RUN.md`](../WIRING_FIRST_RUN.md).

## Build and upload

Install the official PlatformIO IDE extension in Visual Studio Code, then open
this directory as the project folder. PlatformIO downloads the pinned STM32
platform and SimpleFOC dependency on the first build.

From the PlatformIO toolbar:

1. run **Build**;
2. leave the motor PSU output OFF;
3. connect the Nucleo through its ST-LINK USB connector;
4. run **Upload**;
5. open **Serial Monitor** at `115200 baud`.

Equivalent PlatformIO terminal commands are:

```text
pio run
pio run --target upload
pio device monitor -b 115200
```

## First test sequence

1. Keep the motor PSU OFF and reset the Nucleo.
2. Confirm `AS5600 I2C probe: OK`, a plausible angle, successful driver/motor
   initialization and `STATE WAIT_START`.
3. Rotate the rotor by hand and reset once more at a different position. The
   printed angle should change. Do not continue if it does not.
4. Confirm the firmware says `Motor outputs: DISABLED`.
5. Turn on the PSU at `12 V / 0.50 A`.
6. Press USER B1 once. Expect a short alignment movement, then `STATE RUN` and
   telemetry at 5 Hz.
7. Press USER B1 again. Expect `STATE STOPPED` and an electrically free rotor.
8. A later press returns to RUN without repeating alignment.

If the firmware enters `FAULT`, switch off the motor PSU before changing any
wiring. A LOW level on `nFT` immediately disables motor outputs and enters the
existing `FAULT` state. There is no automatic driver reset or motor restart.

## Serial commands

Commands are newline-terminated ASCII text and are case-insensitive:

```text
HELP
STATUS
START
STOP
SPEED 1.0
SPEED -0.5
```

`SPEED` accepts values from `-5.0` to `+5.0 rad/s`. A negative value requests
the opposite direction. The selected value is retained through `STOP`; outputs
remain disabled until `START` or a USER-button press. The first start after each
reset still performs FOC alignment and therefore moves the rotor.

`STATUS` includes `driver_fault=yes/no`. Low-level APIs for a manual DRV8313
reset and sleep/wake are present in `motor_control`, but no serial commands call
them yet.

## Code layout

- `include/board_config.h`: pins and board electrical assumptions;
- `include/motor_config.h`: all editable motor and first-run limits;
- `src/motor/motor_control.*`: Wire, AS5600 and SimpleFOC details;
- `src/app/app_state.*`: hardware-independent application state machine;
- `src/serial/serial_commands.*`: fixed-buffer, non-blocking command parser;
- `src/main.cpp`: setup, button debounce and low-rate telemetry.

## Verified build

Version `0.3.0` was built successfully on 2026-09-20 with PlatformIO Core
6.2.0, ST STM32 platform 19.7.1, STM32 Arduino Core 2.12.0 and SimpleFOC
2.4.0. The release image uses 66,340 bytes Flash and 3,056 bytes RAM.

Version `0.2.0` was verified on the physical motor in both directions, including
FOC alignment, serial speed commands and stop/restart. The added DRV8313
control/status pins in `0.3.0` still require the bench test.

## Important configuration notes

- `MOTOR_POLE_PAIRS = 7` comes from the seller's `12N14P` marking: 14 rotor
  poles divided by two.
- B1 on this Nucleo is active-low; its polarity is isolated in
  `USER_BUTTON_PRESSED_LEVEL`.
- Motor phase order affects direction, but cannot damage the state-machine
  structure. Do not swap phases while power is applied.
- `initFOC()` is intentionally not called during boot because it moves the
  rotor.
