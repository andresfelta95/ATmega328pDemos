# ATmega328P Demos

Bare-metal C demos and peripheral drivers for the **Microchip (Atmel) ATmega328P**, the 8-bit AVR used on the Arduino Uno and Nano. No Arduino core is used: every driver talks to the hardware registers directly (`DDRx`, `PORTx`, `TWCR`, `UCSR0x`, `ADMUX`, `TCCRnx`, ...).

The project was written in 2023 as part of embedded-systems coursework at NAIT. It uses Microchip Studio and the AVR-GCC toolchain, with the CPU clocked at **16 MHz**.

---

## Contents

- [What's in the repository](#whats-in-the-repository)
- [Hardware](#hardware)
- [Demos](#demos)
- [Driver library (`lib/`)](#driver-library-lib)
- [Building and flashing](#building-and-flashing)
- [Known issues](#known-issues)
- [Roadmap](#roadmap)
- [Credits and licensing](#credits-and-licensing)

---

## What's in the repository

```
ATmega328pDemos/
├── Makefile                      Command-line build (avr-gcc) and upload (avrdude)
├── lib/                          Reusable peripheral drivers
│   ├── inc/                      Public headers
│   │   ├── I2C.h                 TWI (I²C) master
│   │   ├── sci.h                 USART0 (serial)
│   │   ├── timer.h               Timer1 output compare, Timer0 fast PWM
│   │   ├── atd.h                 10-bit ADC
│   │   ├── LM75A.h               LM75A temperature sensor (I²C)
│   │   ├── SSD1306.h             SSD1306 OLED display (I²C)
│   │   └── NeoPix.h              WS2812 / NeoPixel LEDs (bit-banged)
│   └── src/                      Driver implementations (*.c)
│
└── Demos/                        One folder per Microchip Studio project
    ├── GccDemo01/                Blink an LED
    ├── GCC_NeoPix/               Cycle colours on a 12-LED NeoPixel ring/strip
    └── GccI2CV01/                VL53L1X laser distance sensor → OLED display
        └── GccI2CV01/
            ├── main.c
            ├── I2C328P.c         (copy of lib/src/I2C328P.c)
            └── API/              STMicroelectronics VL53L1X Ultra-Lite Driver
                ├── core/         ST driver (unmodified, v3.5.2)
                └── platform/     I²C glue written for this project
```

The double folder level (`Demos/X/X/`) is the layout Microchip Studio creates: the solution sits in the outer folder and the project in the inner one. The `.atsln` and `.cproj` files themselves are **not** committed (they are listed in `.gitignore`). You can use the `Makefile` or recreate the projects: see [Building and flashing](#building-and-flashing).

---

## Hardware

| Part | Role | Interface | Address / pin |
|---|---|---|---|
| ATmega328P @ 16 MHz | MCU (bare chip, Arduino Uno or Nano) | n/a | n/a |
| LED + resistor | Blink demo | GPIO | **PB0** (Arduino D8) |
| WS2812B NeoPixel ring/strip (12 LEDs) | NeoPixel demo | 1-wire, 800 kHz | **PD5** (Arduino D5) |
| SSD1306 OLED, 128×32 | Text/graphics output | I²C | `0x3C` |
| ST VL53L1X time-of-flight sensor | Distance measurement | I²C | `0x29` |
| NXP LM75A temperature sensor | Driver only, no demo yet | I²C | `0x48` |

### ATmega328P pin cheat-sheet

| Function | AVR pin | Arduino Uno/Nano pin |
|---|---|---|
| I²C SDA / SCL | PC4 / PC5 | A4 / A5 |
| USART0 RX / TX | PD0 / PD1 | D0 / D1 |
| Timer0 PWM OC0A | PD6 | D6 |
| NeoPixel data | PD5 | D5 |
| Blink LED | PB0 | D8 (the Uno's built-in LED is on D13 / PB5) |
| ADC0–ADC5 | PC0–PC5 | A0–A5 |

> The SSD1306, VL53L1X and LM75A all share the same I²C bus. Most breakout boards already have pull-up resistors on SDA and SCL. If yours don't, add 4.7 kΩ pull-ups to VCC.

---

## Demos

### 1. `GccDemo01`: LED blink

**File:** `Demos/GccDemo01/GccDemo01/main.c` (162 bytes of flash)

The "hello world" of microcontrollers. It configures **PB0** as an output (`DDRB |= 1`) and toggles it every 500 ms with `PORTB ^= 1` and `_delay_ms(500)`. It uses no library code and is a minimal way to check the toolchain, programmer and clock setup.

### 2. `GCC_NeoPix`: NeoPixel colour cycle

**Files:** `Demos/GCC_NeoPix/GCC_NeoPix/main.c`, `lib/src/NeoPix.c` (≈ 410 bytes of flash)

This demo drives a strip or ring of **12 WS2812B LEDs** on **PD5**. Every second it sets all LEDs to a new colour (blue → green → purple) and pushes the frame out.

How the driver works:

- A 36-byte frame buffer (`12 LEDs × 3 bytes`) stores each LED's colour in the **GRB** order WS2812 chips expect.
- `neopixel_set_color()` and `neopixel_set_color_all()` only change the buffer.
- `neopixel_update()` bit-bangs the buffer out MSB-first at 800 kHz. Each bit starts with a high pulse whose length encodes the value: a short pulse is a `0`, a long pulse is a `1`. The bit loop is written in cycle-counted inline assembly for 16 MHz:

  | Bit | High | Low | Period |
  |---|---|---|---|
  | `0` | 6 cycles (375 ns) | 14 cycles | 20 cycles (1.25 µs) |
  | `1` | 13 cycles (812 ns) | 7 cycles | 20 cycles (1.25 µs) |

  Interrupts are blocked while the frame is sent, so an ISR can't stretch a pulse. Afterwards the line is held low for 300 µs so the LEDs latch the new colours.

The `main.c` file also contains a commented-out experiment that used Timer1 interrupts to generate the pulses.

### 3. `GccI2CV01`: VL53L1X distance sensor on an OLED

**Files:** `Demos/GccI2CV01/GccI2CV01/` + `lib/src/I2C328P.c` + `lib/src/SSD1306.c` (≈ 7.4 KB flash, 849 B RAM)

This is the main demo. It reads distance from an **ST VL53L1X** laser time-of-flight sensor and shows it on a **128×32 SSD1306 OLED**, both on the same I²C bus at 100 kHz.

Program flow:

1. `I2C_Init(F_CPU, I2CBus100)` sets up the TWI peripheral. `I2C_Scan()` then probes every 7-bit address and records which devices answer (useful when debugging wiring).
2. `SSD1306_DispInit()`, `SSD1306_DisplayOn()` and `SSD1306_Clear()` bring up the display.
3. The program polls `VL53L1X_BootState()` until the sensor reports that it has booted.
4. It configures the sensor:
   - `VL53L1X_SensorInit()` loads ST's default configuration.
   - `VL53L1X_SetDistanceMode(0, 1)` selects **short** mode (up to ≈ 1.3 m, better ambient-light immunity).
   - `VL53L1X_SetTimingBudgetInMs(0, 500)` and `VL53L1X_SetInterMeasurementInMs(0, 500)` set slow, accurate measurements twice per second.
   - `VL53L1X_StartRanging()` starts continuous measurement.

   If any of these calls fails, the display shows `VL53L1X init error` and the program stops.
5. Main loop: wait for `CheckForDataReady`, read `GetRangeStatus` and `GetDistance` (in mm), then `ClearInterrupt`. The loop prints **`Distance:  12.3 cm`** on text row 3, or `No valid range (n)` if the range status isn't 0, and waits 500 ms. Each line is padded to the full row width, so it overwrites the previous reading without clearing (and flickering) the display.

#### Porting ST's driver to the AVR

ST ships the VL53L1X **Ultra-Lite Driver (ULD, STSW-IMG009)** as portable C code. To run it on a new platform you implement a small set of register-access functions in `vl53l1_platform.c`. This project implements them on top of the `lib/` I²C driver:

| Platform function | What it does on the bus |
|---|---|
| `VL53L1_WrByte / WrWord / WrDWord / WriteMulti` | START, `0x29`+W, 16-bit register index (MSB first), the data bytes (MSB first), STOP |
| `VL53L1_RdByte / RdWord / RdDWord / ReadMulti` | START, `0x29`+W, 16-bit index, STOP; then START, `0x29`+R, read the bytes (ACK each one except the last, which gets NACK), STOP |
| `VL53L1_WaitMs` | Busy-waits with `_delay_ms(1)` in a loop |

All of them share two helpers, `VL53L1_Write()` and `VL53L1_Read()`, so the bus sequence is written only once. The commit history ("Debuggin VL53L1X library", "Bug fixed") records getting this layer working, mainly the byte order of 16-bit reads.

---

## Driver library (`lib/`)

All drivers are written in plain C against the ATmega328P register set. Each init function also clears the module's bit in the **Power Reduction Register (`PRR`)**, because the start-up code can leave peripherals powered down.

### `I2C.h` / `I2C328P.c`: TWI master (polling)

| Function | Description |
|---|---|
| `int I2C_Init(unsigned long busHz, I2C_BusRate rate)` | Sets `TWBR` for 100 kHz (`I2CBus100`) or 400 kHz (`I2CBus400`) and enables TWI. Returns `-1` if the rate can't be reached. |
| `int I2C_Start(unsigned char addr7, int bRead)` | Sends START or repeated START plus the address and R/W bit. Returns `0` on ACK, `<0` on error. |
| `int I2C_Write8(unsigned char data, int bStop)` | Writes one byte. Optionally sends STOP. |
| `int I2C_Read8(unsigned char *data, int bAck, int bStop)` | Reads one byte, replies ACK (more bytes wanted) or NACK (last byte), and optionally sends STOP. |
| `void I2C_Scan(unsigned char results[128])` | Probes addresses `0x01`–`0x7E`. `results[addr] = addr` if a device ACKs, otherwise 0. |

Helper constants: `I2C_READ/WRITE`, `I2C_ACK/NACK`, `I2C_STOP/NOSTOP`. Each step checks the TWI status codes (`0x08`, `0x10`, `0x18`, `0x28`, `0x40`, `0x50`, `0x58`) from the datasheet.

### `SSD1306.h` / `SSD1306.c`: OLED display

The driver keeps a **back-buffer** in RAM (512 B for 128×32, 1 KB for 128×64) plus a "dirty" flag per 8-pixel page. Drawing calls only change the buffer. `SSD1306_Render()` sends only the dirty pages over I²C.

| Group | Functions |
|---|---|
| Management | `SSD1306_DispInit`, `SSD1306_Clear`, `SSD1306_Render`, `SSD1306_IsDirty`, `SSD1306_DisplayOn/Off`, `SSD1306_Noise` |
| Text (5×7 font in flash, 21 columns × 4 rows on 128×32) | `SSD1306_CharXY(col, row, ch)`, `SSD1306_StringXY(col, row, str)`. Character `'\x1F'` draws a "°C" symbol. |
| Graphics | `SSD1306_SetPixel(x, y)`, `SSD1306_Line(x0, y0, x1, y1)`, `SSD1306_Circle(x, y, r)`, `SSD1306_SetPage(page, PROGMEM_buffer)` |

Configuration happens in `SSD1306.h`: `_SSD1306_ADDRESS` (default `0x3C`) and `_SSD1306_DisplaySize128x32` / `_SSD1306_DisplaySize128x64`. The font table is stored in flash with `PROGMEM` and copied with `memcpy_P`, which keeps it out of the 2 KB of RAM.

### `NeoPix.h` / `NeoPix.c`: WS2812 LEDs

| Function | Description |
|---|---|
| `neopixel_init()` | Makes the data pin (`PD5`) an output |
| `neopixel_set_color(i, r, g, b)` / `neopixel_set_color_all(r, g, b)` | Writes colours into the frame buffer |
| `neopixel_turn_off(i)` / `neopixel_turn_off_all()` | Sets one or all LEDs to black |
| `neopixel_update()` | Sends the buffer to the LEDs (interrupts blocked during the frame), then latches it |
| `neopixel_send_pulse()` | Holds the data line low for 300 µs so the LEDs latch the frame (called by `neopixel_update()`) |

The pin and LED count are set by `NEOPIXEL_PORT`, `NEOPIXEL_DDR`, `NEOPIXEL_PIN` and `NEOPIXEL_NUM_LEDS` (currently 12) in the header. The timing only works at 16 MHz, so `NeoPix.c` stops the build with an `#error` if `F_CPU` is anything else.

### `sci.h` / `sci328P.c`: USART0 serial

| Function | Description |
|---|---|
| `int SCI0_Init(unsigned long busHz, unsigned long baud, int bRxInt)` | 8N1, asynchronous. Computes a rounded `UBRR0` and can enable the RX interrupt. Returns `-1` if the baud rate doesn't fit. |
| `SCI0_BSend(byte)` / `SCI0_TxString(str)` | Blocking transmit |
| `SCI0_Tx16H(value, newline)` | Prints a 16-bit value as `0x1234` |
| `int SCI0_RxByte(&byte)` | Non-blocking receive. Returns `0` if a byte was read. |

`sci.h` includes a template `ISR(USART_RX_vect)` you can copy into your program.

### `timer.h` / `timer328P.c`: timers

| Function | Description |
|---|---|
| `Timer_Init(Timer_Prescale pre, unsigned int offset)` | Timer1 in normal mode with the **Output Compare A interrupt** enabled, for periodic ticks. The ISR must add `offset` to `OCR1A` again (template in `timer.h`). Example: prescale 64 with offset 250 gives a 1 ms tick at 16 MHz. |
| `Timer_F_PWM0(chan, clksel, pol)` | Timer0 **fast PWM** on **OC0A (PD6)** or **OC0B (PD5)**, inverting or non-inverting, starting at 50 % duty. Change duty with `OCR0A` / `OCR0B`. Both channels can run at once. No interrupt is enabled. |

### `atd.h` / `atd328P.c`: ADC

| Function | Description |
|---|---|
| `AtoD_Init(channel)` | Powers up the ADC in **free-running** mode with a ÷128 prescaler (125 kHz ADC clock at 16 MHz) and selects the channel |
| `AtoD_SetChannel(channel)` | Switches `ADMUX` to another channel, picks the voltage reference, and disables the digital input buffer on that pin (`DIDR0`, ADC0–ADC5 only) |

The reference is **AVcc** for the input channels and the **internal 1.1 V** reference for `AtoD_TempSensor`, as the datasheet requires.

There is no read function: read the 10-bit result directly from the `ADC` register.

### `LM75A.h` / `LM75A.c`: temperature sensor

| Function | Description |
|---|---|
| `int LM75A_ReadTemp(unsigned int *raw)` | Reads the 16-bit temperature register at I²C address `0x48` |
| `float LM75A_GetTempF()` | Converts the 11-bit two's-complement value to **°C** (0.125 °C resolution). Returns `-300` on an I²C error. The trailing `F` means *float*, not Fahrenheit. |

---

## Building and flashing

### Option A: Microchip Studio (the original workflow)

The original `.atsln` and `.cproj` files are not in the repository, so you have to recreate each project:

1. **File → New → Project → GCC C Executable Project**, device **ATmega328P**.
2. Replace the generated `main.c` with the demo's `main.c`.
3. Add the needed driver sources with **Add → Existing Item → Add As Link**, so the files stay shared in `lib/src`:
   - `GCC_NeoPix`: `lib/src/NeoPix.c`
   - `GccI2CV01`: `lib/src/SSD1306.c`, the demo's local `I2C328P.c`, and everything under `API/`
4. **Project Properties → Toolchain → AVR/GNU C Compiler → Directories**: add `lib/inc`. For `GccI2CV01`, also add `API/core` and `API/platform`.
5. Build (F7) and program with your tool (Atmel-ICE, AVRISP mkII, Arduino bootloader through avrdude, ...).

### Option B: command line with the `Makefile`

Install the toolchain:

- **Debian/Ubuntu:** `sudo apt install gcc-avr avr-libc binutils-avr avrdude make`
- **macOS:** `brew tap osx-cross/avr && brew install avr-gcc avrdude`
- **Windows:** the toolchain that ships with Microchip Studio (add its `bin` folder to `PATH`) plus GNU Make, or WinAVR

Then, from the repository root:

```bash
make                 # build all demos into build/<demo>/<demo>.hex and compile every driver in lib/
make tof             # build one demo: blink, neopix or tof
make size            # flash and RAM use of each demo
make flash-tof       # build and upload with avrdude
make clean           # delete build/
make help            # list the targets
```

`make` also compiles the drivers that no demo uses yet (`sci`, `timer`, `atd`, `LM75A`), so a mistake in them shows up straight away. It was tested with avr-gcc 7.3 and builds with no warnings at `-Wall`.

| Demo target | Source | Flash | RAM |
|---|---|---|---|
| `blink` | `Demos/GccDemo01` | 162 B | 0 B |
| `neopix` | `Demos/GCC_NeoPix` | 412 B | 36 B |
| `tof` | `Demos/GccI2CV01` | 7374 B | 849 B |

The clock is set once with `F_CPU` (default `16000000UL`). Every source file falls back to 16 MHz when `F_CPU` isn't defined, which is the case in Microchip Studio unless you add it under **Symbols**.

### Flashing

`make flash-<demo>` runs avrdude with settings you can override on the command line:

| Variable | Default | Notes |
|---|---|---|
| `PROGRAMMER` | `arduino` | `usbasp`, `avrisp2`, `atmelice_isp`, ... for an ISP programmer |
| `PORT` | `/dev/ttyACM0` | `COM3` on Windows, `/dev/ttyUSB0` for most Nano clones, `usb` for USB programmers |
| `BAUD` | `115200` | `57600` for a Nano with the old bootloader |

```bash
make flash-tof                                   # Arduino Uno
make flash-neopix PORT=/dev/ttyUSB0 BAUD=57600   # Nano, old bootloader
make flash-blink PROGRAMMER=usbasp PORT=usb      # USBasp, no bootloader
```

This is the same as calling avrdude directly:

```bash
avrdude -c arduino -p m328p -P /dev/ttyACM0 -b 115200 -U flash:w:build/tof/tof.hex:i
```

---

## Known issues

### Fixed

A code review in November 2026 found these bugs. They are fixed in the current code but have **not been re-tested on hardware** yet.

| Where | What was wrong | Fix |
|---|---|---|
| `GccI2CV01/main.c` | `char distance[10]` was too small for `"Distance: 123.4 cm"`, so `sprintf` wrote past the end (the compiler warned about it). | 22-byte line buffer and `snprintf` |
| `GccI2CV01/main.c` | `_DataReady` was never set back to 0, so after the first reading the loop stopped waiting for new data. | Reset before each wait |
| `vl53l1_platform.c` `RdDWord` | `byte << 24` was computed in 16-bit `int`, so the top two bytes were lost (compiler warning). | Cast to `uint32_t` before shifting |
| `vl53l1_platform.c` `ReadMulti` | ACK and NACK were swapped: every byte but the last was NACKed and the last was ACKed. | ACK all bytes except the last |
| `vl53l1_platform.c` `WrByte` | Never sent STOP, so the bus was never released. | All writes now end with STOP |
| `vl53l1_platform.c` `WaitMs` | Empty stub | Waits with `_delay_ms(1)` in a loop |
| `SSD1306.c` `SetPage` (128×32) | Allowed `page == 4`, writing 128 bytes past the 512-byte buffer | Limit is now page 3 |
| `SSD1306.c` `CharXY` | Column 21 ran off the 128-pixel row and past the buffer on the last row | Columns wrap at 21 |
| `NeoPix.c` | A "1" bit was long only because of a slow 64-bit counter increment (≈ 4.4 µs). Timing depended on the optimiser and on interrupts. | Cycle-counted assembly, interrupts blocked, 300 µs latch |
| `timer328P.c` `Timer_F_PWM0` | Enabled the Timer0 overflow interrupt with no ISR, which **resets** the MCU on `sei()`. `OC0B` was accepted but ignored. | No interrupt; `OC0B` implemented |
| `atd328P.c` | Used the external AREF pin as reference (not connected on an Arduino). The temperature sensor needs the 1.1 V reference. | AVcc by default, 1.1 V for `AtoD_TempSensor` |

> **Behaviour changes to be aware of:**
> - `Timer_F_PWM0` no longer enables `TOIE0`. If you used `ISR(TIMER0_OVF_vect)` with it, set `TIMSK0 |= (1 << TOIE0)` yourself.
> - The ADC no longer uses the AREF pin. If your board drives AREF with its own reference, change `AtoD_SetChannel`.

### Still open

- **No I²C timeouts.** Every `while (!(TWCR & (1<<TWINT)))` waits forever, so an unplugged sensor or a stuck bus freezes the program. The I²C error paths also return without sending STOP.
- **`I2C_Scan` results aren't shown.** `GccI2CV01` collects them but doesn't display them.
- **Display size is hard-wired.** `SSD1306.h` always defines `_SSD1306_DisplaySize128x32`, so defining 128×64 from the build adds a second set of definitions and the build fails. Each display function is also written twice, once per size. A single `SSD1306_PAGES` constant would remove the duplication.
- **Most headers have no include guards** (`I2C.h`, `sci.h`, `timer.h`, `atd.h`, `LM75A.h`, `SSD1306.h`).
- **Duplicated `I2C328P.c`.** `Demos/GccI2CV01/GccI2CV01/I2C328P.c` is identical to `lib/src/I2C328P.c` (the `Makefile` uses the `lib/` copy). The library copy also starts with a UTF-8 BOM.
- **Hard-coded VL53L1X address.** The platform layer uses `0x29` and ignores the `dev` parameter, so two sensors (or a re-addressed one) can't be used.
- **Leftover code.** There are large commented-out blocks in `GCC_NeoPix/main.c` and `SSD1306.c`.
- **`.gitignore` hides the project files** (`*.cproj`, `*.atsln`). On a case-insensitive file system its `*.s` rule would also ignore hand-written assembly (`.S`).

---

## Roadmap

- [x] Fix the bugs found in the code review
- [x] Add a `Makefile` so every demo builds with one command
- [ ] Re-test all three demos on hardware after the fixes
- [ ] Add I²C timeouts and send STOP on errors
- [ ] Commit the Microchip Studio `.atsln` / `.cproj` files, or document the project settings
- [ ] Add a GitHub Actions workflow that runs `make`
- [ ] Add demos for the drivers that don't have one yet: **UART echo**, **ADC → serial**, **Timer1 1 ms tick**, **PWM LED dimming**, **LM75A thermometer on the OLED**
- [ ] Add wiring diagrams or photos and a short video/GIF of each demo in `docs/`
- [ ] Attach prebuilt `.hex` files to GitHub Releases
- [ ] Add a `LICENSE` file (see below)

---

## Credits and licensing

| Component | Author | License |
|---|---|---|
| `I2C`, `sci`, `timer`, `atd`, `LM75A`, `SSD1306` drivers | Simon Walker, NAIT (course material). SSD1306 character map by Chelsea Walker, 128×32 variant by Diarmid Rendell. | Not specified |
| `NeoPix` driver, all demos, VL53L1X platform port | Andres Tangarife | Not specified |
| VL53L1X Ultra-Lite Driver (`API/core`, platform templates) | STMicroelectronics (STSW-IMG009) | BSD 3-Clause (see file headers) |

The repository has no top-level license yet. Before adding one, check with the original authors whether the NAIT course drivers can be redistributed under it. ST's code must keep its BSD-3 notice either way.
