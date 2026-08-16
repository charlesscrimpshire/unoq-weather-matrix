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

Rule: **red when temp >= 99 °F, blue otherwise** (so 99 °F = red, 98 °F = blue).
Threshold lives in one line in each of `unoq-weather-matrix.ino` and `temp_leds.py`.

## Data source

Started with open-meteo's forecast; the `current_weather` value wobbles between
fetches, so the display and LEDs disagreed. Switched to **NWS observation station KLUL**
(Laurel/Noble Field, nearest to Laurel, MS at 31.69,-89.13):

- Endpoint: `https://api.weather.gov/stations/KLUL/observations/latest`
- Value: `properties.temperature.value` (Celsius float; may be `null` — treat as missing)
- Requires a `User-Agent` header.
- Observations refresh ~every minute, matching a real thermometer.

## Router socket-leak fix (arduino-router v0.10.0)

While running for hours with no network (NO NET / `-1000` fallback), the companion
process (Go `arduino-router`) slowly leaked sockets: after ~4.7 h, `lsof -i` showed
671 open fds and a growing pile of `CLOSE-WAIT` entries, until the process crashed.

Root cause (upstream `internal/network-api/network-api.go`, `tcpRead`): when the TLS
read returned `(n>0, io.EOF)`, the code **discarded the received bytes and returned an
RPC error** instead of delivering them. The MCU side sees the error at
`tcp_client.h:216-218` and sets `_connected = false`, so `client.stop()` skips the
`tcp/close` RPC — leaving the router's `liveConnections` entry open forever
(CLOSE-WAIT, fd never freed).

Fix (patched locally, upstream untouched):
- On timeout or EOF **with data** (`n > 0`): return the bytes `res(buffer[:n], nil)`.
- On clean EOF (`n == 0`): `delete(liveConnections, id)` and `_ = c.Close()`.

Verified on the board: after reboot + re-upload, fd count stayed flat (11–12 over
2.5 min) with no accumulating CLOSE-WAIT, even with repeated weather fetches.

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
