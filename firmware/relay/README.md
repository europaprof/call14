# ESPHome hall-call relay

This configuration is for the tested XY-WF36V/ESP8285 board. It exposes one
momentary Home Assistant button and keeps the physical relay switch internal.

## Verified pins

| Function | ESP pin |
|---|---:|
| Relay | GPIO4 |
| Green LED | GPIO13 |
| On-board button | GPIO12, active low |

The relay pulse is 500 ms and always returns to off. Verify every pin on your
own board revision before applying power; similar-looking modules may differ.

## Build

1. Install ESPHome.
2. Copy `secrets.example.yaml` to `secrets.yaml` and replace every placeholder.
3. Validate with `esphome config lift-button.yaml`.
4. For the first serial flash, connect 3.3 V UART safely and hold GPIO0 to GND
   during reset/power-up to enter the ESP8266 bootloader.
5. Later releases can be installed with ESPHome OTA.

Do not power this module from AC. Measure the hall-button circuit before choosing
a power supply. The relay contact must be isolated and connected only in parallel
with the normal hall-call contact, subject to building rules and authorization.
