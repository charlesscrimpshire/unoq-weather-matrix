#!/usr/bin/env bash
set -euo pipefail

# Push + start the LED1/LED2 companion (temp_leds.py) on the board's Linux side.
cd "$(dirname "$0")/.."

ADB="${ADB:-$HOME/.local/share/arduino_applab_workspace/arduino15/data/packages/arduino/tools/adb/32.0.0/adb}"
REMOTE="/home/arduino/temp_leds.py"

# Separate adb calls: 'pkill -f' would otherwise match the wrapping adb shell.
"$ADB" shell "pkill -f temp_leds" 2>/dev/null || true
"$ADB" push temp_leds.py "$REMOTE"
"$ADB" shell "setsid nohup python3 $REMOTE >/home/arduino/temp_leds.log 2>&1 &"
sleep 1
"$ADB" shell "pgrep -af '[t]emp_leds'"
