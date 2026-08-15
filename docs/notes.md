# Project Notes

Hands-on notes from building and debugging this project on the Arduino UNO Q.

## Board probing

- Dual-chip: STM32U585 MCU (Cortex-M33, 2 MB flash / 786 KB SRAM) + Qualcomm QCM2290
  "Imola" Linux companion (4x Kryo-V2 @ 2.0 GHz, Adreno GPU, Debian Linux, ~3.6 GB RAM).
- Linux userspace user is `arduino`; root shell via `adb root` (serial `/dev/ttyHS1`
  115200; adb USB).
- No user-accessible temperature sensors on the board — the SoC thermal zones are the
  only local temps, so weather comes from the network.

## LED architecture (4 RGB LEDs, two control planes)

| LED | Path | Control |
|-----|------|---------|
| LED1 | `red:user` / `green:user` / `blue:user` | Linux sysfs, 1 = on |
| LED2 | `red:panic` / `green:wlan` / `blue:bt` | Linux sysfs, 1 = on |
| LED3 | `LED3_R/G/B` pins | MCU, `analogWrite` (PWM), **inverted** |
| LED4 | `LED4_R/G/B` pins | MCU, `digitalWrite`, **active-low** |

Rule: **red when temp > 95 °F, blue otherwise** (strict `>`, so 95 °F = blue).
Threshold lives in one line in each of `unoq-weather-matrix.ino` and `temp_leds.py`.

## Data source

Started with open-meteo's forecast; the `current_weather` value wobbles between
fetches, so the display and LEDs disagreed. Switched to **NWS observation station KLUL**
(Laurel/Noble Field, nearest to Laurel, MS at 31.69,-89.13):

- Endpoint: `https://api.weather.gov/stations/KLUL/observations/latest`
- Value: `properties.temperature.value` (Celsius float; may be `null` — treat as missing)
- Requires a `User-Agent` header.
- Observations refresh ~every minute, matching a real thermometer.

## Gotchas

- `BridgeTCPClient::connect()` / `connectSSL()` return **0 on success, -1 on failure** —
  check `>= 0`, NOT truthiness. The classic `if (client.connect(...))` inverts the logic
  and makes every fetch fail with `-1000` (and leaks idle sockets on the router).
- NWS response is ~4.6 KB and `"temperature"` sits past byte 2000 — the fetch buffer was
  raised 1200 → 8000 bytes.
- `connectSSL(host, 443, "")` works: the router (Go binary) verifies against the board's
  system CA store. `/etc/ssl/certs/` contains `ISRG_Root_X1.pem` / `ISRG_Root_X2.pem`;
  `/etc/pki/tls/certs/ca-bundle.crt` and `/etc/ssl/ca-bundle.pem` do NOT exist. A large
  embedded PEM string is unsafe (router RPC message-size limit) — don't embed certs.
- The sketch's fetch read loop runs the full 10 s window before `client.stop()`.
- `pkill -f` self-matches the wrapping adb shell — use separate adb calls or the
  `[t]emp_leds` bracket trick when restarting the companion.
- Zephyr libc: avoid `fmodf` / `atof` / `strtod` (link errors with `__errno`).
- Static 2-digit temp uses `Font_4x6`; ≥100 °F scrolls instead.
- Useful compile/upload FQBN is `arduino:zephyr:unoq` (not `...stm32u585xx`).
