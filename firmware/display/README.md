# Call14 display firmware

Production firmware for a JUZIPi SD_PRO-style clock based on an ESP8266 ESP-12F and a
240x240 ST7789. It includes an animated clock/elevator preview, GPIO4 call
button, Wi-Fi, MQTT/Home Assistant transport, status and firmware-update pages,
recovery mode, and ArduinoOTA.

## Pin assumptions — verify on the PCB

| Function | ESP8266 GPIO | Status / warning |
|---|---:|---|
| ST7789 SCLK | 14 | Verified working configuration |
| ST7789 MOSI | 13 | Verified working configuration |
| ST7789 CS | 15 | Boot-strap pin; must remain low at boot |
| ST7789 DC | 0 | Boot-strap pin; do not load externally |
| ST7789 RST | 2 | Boot-strap pin; do not load externally |
| Backlight | 5 | Active low |
| Call button (`O` to `G`) | 4 | Verified by continuity test; `INPUT_PULLUP` |

GPIO0 and GPIO2 must remain high during boot; GPIO15 must remain low. The rear
`O` pad was continuity-tested to GPIO4; connect a normally-open button between
`O` and `G`.

## Observed hardware and vendor firmware

Inspection of the photographed unit confirms an ESP-12F module and PCB marking
`2025/11/10 V1.1`. The rear side has an unpopulated switch footprint and a
three-pad header marked `G V O`. On this unit, continuity testing confirmed that
`O` reaches GPIO4. Similar boards should still be measured before treating that
as universal. The display flex is marked approximately `WA548C049I-10Z`.

The official vendor repository contains compiled firmware and a user manual,
but no source code or schematic:

<https://github.com/JUZIPi-tech/SD_PRO>

Vendor firmware V1.0.6 is a 488,304-byte ESP8266 OTA/application image. Its
image header declares DIO flash mode, 40 MHz flash frequency, and 4 MB flash
size. Those values agree with this project's current `esp12e`/4 MB build
profile. Keep an official vendor BIN locally as an additional rollback option,
but do not assume it replaces a complete UART flash backup.

## Build

1. Copy `include/secrets.example.h` to `include/secrets.h` and edit all values.
2. Build with `pio run`.
3. The firmware binary is `.pio/build/esp12f/firmware.bin`.

The default `change-me` OTA password is only a build-safe placeholder and must
not be used for a permanently installed device.

## Recovery and OTA

On successful Wi-Fi connection, open `http://sd-pro-lift-display.local/status`
(mDNS availability depends on the client) or use the IP printed to serial and
shown on the display. Browser upload is at `/update`; use HTTP Basic user
`admin` and `OTA_PASSWORD` from `secrets.h`.

Temporary HA/test endpoints:

- `/api/elevator?state=calling&floor=14&direction=down&eta=30`
- `/api/elevator?state=moving&floor=12&direction=down&eta=18`
- `/api/elevator?state=arrived&floor=1`
- `/api/elevator?state=idle`
- `/api/motion?location=Floor%2014`

If Wi-Fi credentials are absent or connection fails for 15 seconds, the device
creates the recovery AP `SD-PRO-Recovery` using `RECOVERY_AP_PASSWORD` from
`secrets.h`. Connect to it and open `http://192.168.4.1/update`. This fallback
keeps browser firmware upload reachable without UART.

ArduinoOTA is also enabled under hostname `sd-pro-lift-display` and uses
`OTA_PASSWORD` from `secrets.h`.

## Before the first flash

- **Do not trust the enclosure or product name alone.** Similar clocks exist
  with different boards and display wiring. Confirm the display pins before
  replacing the factory firmware over OTA.
- OTA can successfully install firmware that boots with a black screen when the
  pin map or controller is wrong. That happened here: the case then had to be
  reopened and tiny ESP8266 UART pads soldered for recovery.
- Inspect or continuity-test every display connection in the table.
- For the photographed `2025/11/10 V1.1` board, use the verified table and the
  `platformio.ini` in this directory.
- Keep UART access available for the first installation. Browser OTA cannot
  preserve the vendor firmware unless you already have a restorable backup.
- Verify flash size/mode from the existing device or module markings. The
  current profile uses 4 MB, DIO, 40 MHz, matching the official V1.0.6 image
  header inspected on 2026-08-29.
