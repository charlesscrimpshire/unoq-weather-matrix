# UNO Q Weather Matrix

Personal Arduino UNO Q weather display for Scrimptech: scrolls "Scrimptech" and "Laurel MS",
shows the live temperature on the built-in 12x9 LED matrix, and drives all four onboard RGB
LEDs as a hot/cold indicator — all from **real observed weather** at Laurel/Noble Field (KLUL).

## What This Is

The Arduino UNO Q is a dual-chip board (STM32U585 MCU + Qualcomm QCM2290 Linux companion).
This project uses both halves:

- The **MCU** runs the sketch: scrolling text on the LED matrix and the static 5 s temperature
  display, plus driving LED3/LED4.
- The **Linux side** runs a small Python companion that drives LED1/LED2 over sysfs.

Temperature comes from the NOAA/NWS observation station KLUL (Laurel/Noble Field) — actual
measured air temperature, no forecast interpolation, so the display tracks a real thermometer.

## Features

- Matrix scroll: `Scrimptech` → `Laurel MS` → live temp (static 5 s, `Font_4x6`)
- LED rule: **red when temp > 95 °F, blue otherwise** (threshold in one line in both files)
- TLS fetch from `api.weather.gov` via the board's system CA store (no embedded cert)
- Fallback to open-meteo over plain HTTP if TLS is unreachable
- Linux companion polls every 30 s and flips LED1/LED2 to match

## Hardware

- Arduino UNO Q — STM32U585 MCU (Cortex-M33, 2 MB flash / 786 KB SRAM) + Qualcomm
  QCM2290/QRB2210 "Imola" Linux companion (4x Kryo-V2 @ 2.0 GHz, Debian, 3.6 GB RAM)
- Built-in 12x9 monochrome blue LED matrix
- 4 onboard RGB LEDs:
  - LED1 / LED2: Linux-controlled (`/sys/class/leds`, on/off)
  - LED3 / LED4: MCU pins `LED3_R/G/B` + `LED4_R/G/B` (PWM / digital, active-low)

## Getting Started

1. Build and flash the sketch:

   ```sh
   ./scripts/flash.sh
   ```

2. Start the Linux companion (drives LED1/LED2 on the board's Linux side):

   ```sh
   ./scripts/run_temp_leds.sh
   ```

3. Power on — the matrix scrolls the sequence and the LEDs reflect the temperature.

## Project Layout

| Path                    | Purpose                                              |
|-------------------------|------------------------------------------------------|
| `unoq-weather-matrix.ino` | Main sketch: matrix + LED3/LED4 temp indicator   |
| `temp_leds.py`          | Linux companion for LED1/LED2 (polls KLUL every 30 s) |
| `docs/notes.md`         | Hands-on notes: board internals, wiring, gotchas    |
| `scripts/flash.sh`      | arduino-cli compile + upload                        |
| `scripts/run_temp_leds.sh` | Push + start the companion on the board          |

## Resources

- [Arduino UNO Q](https://docs.arduino.cc/hardware/uno-q/)
- [NWS API](https://www.weather.gov/documentation/services-web-api)
- [KLUL station](https://api.weather.gov/stations/KLUL/observations/latest)
