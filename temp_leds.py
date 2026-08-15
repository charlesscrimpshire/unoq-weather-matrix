#!/usr/bin/env python3
import time
import urllib.request
import json

URL = "https://api.weather.gov/stations/KLUL/observations/latest"
LEDS_RED = {"red:user": 1, "green:user": 0, "blue:user": 0,
            "red:panic": 1, "green:wlan": 0, "blue:bt": 0}
LEDS_BLUE = {"red:user": 0, "green:user": 0, "blue:user": 1,
             "red:panic": 0, "green:wlan": 0, "blue:bt": 1}


def set_leds(mapping):
    for name, value in mapping.items():
        with open("/sys/class/leds/%s/brightness" % name, "w") as f:
            f.write(str(value))


while True:
    try:
        req = urllib.request.Request(URL, headers={"User-Agent": "arduino-unoq/1.0"})
        with urllib.request.urlopen(req, timeout=15) as r:
            data = json.loads(r.read().decode())
        value = data["properties"]["temperature"]["value"]
        if value is not None:
            f = int((value * 9 + 2) / 5 + 32)
            set_leds(LEDS_RED if f > 95 else LEDS_BLUE)
    except Exception:
        pass
    time.sleep(30)
