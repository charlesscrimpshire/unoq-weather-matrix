# unoq-weather-matrix

Arduino UNO Q weather display: scrolls "Scrimptech", then "Laurel MS", then shows the
live temperature (static for 5 s) on the built-in 12x9 LED matrix, while all four onboard
RGB LEDs act as a hot/cold indicator.

## How it works

- Temperature is read from **real observed weather** at Laurel/Noble Field (KLUL),
  via the NOAA/NWS API (`api.weather.gov/stations/KLUL/observations/latest`). No forecast
  interpolation, so the displayed value tracks the actual air temperature.
- LED rule: **red when temp > 95 °F, blue otherwise** (edit the threshold in both files).
- The four onboard LEDs are driven by two independent paths:
  - **LED3 / LED4** (MCU pins `LED3_R/G/B`, `LED4_R/G/B`) are driven directly by the
    sketch with the same value it displays.
  - **LED1 / LED2** (Linux-controlled via `/sys/class/leds`, channels `red:user/green:user/blue:user`
    and `red:panic/green:wlan/blue:bt`) are driven by the companion script
    [`temp_leds.py`](temp_leds.py), which polls the same station every 30 s.

## Files

| File                 | Purpose                                                                 |
|----------------------|-------------------------------------------------------------------------|
| `unoq-weather-matrix.ino` | Main sketch: matrix scroll/display + LED3/LED4 temp indicator.     |
| `temp_leds.py`       | Linux companion: polls KLUL and sets LED1/LED2 brightness.             |

## Notes / gotchas

- `BridgeTCPClient::connect()` / `connectSSL()` return **0 on success, -1 on failure**
  (not the usual 1/0), so check `>= 0`, not truthiness.
- The sketch connects to NWS over TLS (`connectSSL`) and lets the router verify against
  the board's system CA store (`/etc/ssl/certs/ISRG_Root_X1.pem`) — no cert is embedded.
  If TLS is unreachable it falls back to open-meteo over plain HTTP.
- The NWS response is ~4.6 KB and `"temperature"` sits past byte 2000, so the fetch
  buffer is 8 KB.

## Build & upload

```sh
arduino-cli compile --fqbn arduino:zephyr:unoq unoq-weather-matrix
arduino-cli upload -p /dev/ttyACM0 --fqbn arduino:zephyr:unoq unoq-weather-matrix
```

Run the LED1/LED2 companion on the board's Linux side:

```sh
python3 temp_leds.py
```

## Board

Arduino UNO Q — STM32U585 MCU (Cortex-M33) + Qualcomm QCM2290/QRB2210 Linux companion,
Debian Linux, Adreno GPU.
