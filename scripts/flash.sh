#!/usr/bin/env bash
set -euo pipefail

# Compile + upload the UNO Q weather matrix sketch via arduino-cli.
cd "$(dirname "$0")/.."

export ARDUINO_DATA_DIR="${ARDUINO_DATA_DIR:-$HOME/.local/share/arduino_applab_workspace/arduino15/data}"
export ARDUINO_DOWNLOADS_DIR="${ARDUINO_DOWNLOADS_DIR:-$HOME/.local/share/arduino_applab_workspace/arduino15/staging}"
export ARDUINO_SKETCHBOOK_DIR="${ARDUINO_SKETCHBOOK_DIR:-$HOME/.local/share/arduino_applab_workspace/arduino15/user}"

CLI="${ARDUINO_CLI:-$HOME/.local/share/arduino_applab_workspace/resources/arduino/arduino-cli/arduino-cli}"
PORT="${PORT:-/dev/ttyACM0}"
FQBN="arduino:zephyr:unoq"

"$CLI" compile --fqbn "$FQBN" unoq-weather-matrix
"$CLI" upload -p "$PORT" --fqbn "$FQBN" unoq-weather-matrix
