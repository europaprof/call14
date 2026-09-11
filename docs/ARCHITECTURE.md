# Call14 architecture

Call14 is a local-first smart-home accessory built around two independent data
paths. It is intentionally not an elevator controller.

## Control path

```text
Clock button / Xiaomi button / phone
                ↓
        Home Assistant script
                ↓
          ESP relay pulse
                ↓
Original hall-call button contacts
```

The relay uses an isolated normally-open contact wired in parallel with the
existing call button. The original button is neither removed nor placed behind
software. The relay has no connection to movement, doors, brakes, interlocks,
or the elevator controller's internal bus.

The display button publishes `PRESS` to a local MQTT command topic. Home
Assistant converts that event into the same call script used by the original
Xiaomi smart button and the dashboard. The relay firmware limits the physical
activation to a momentary pulse.

## Observation path

```text
Original red indicator
          ↓
Authorized cabin camera
          ↓
Local crop + aligned hybrid recognizer v6
          ↓
MQTT discovery and retained state
          ↓
Home Assistant → Call14 display
```

The recognizer reads a small crop of the video locally and publishes structured
state: floor, direction, movement, display/fault condition, confidence and
availability. Full video is not sent to the clock or an external OCR service.

## Failure boundaries

| Failure | Result |
|---|---|
| Call14 display offline | Xiaomi button, phone and original hall button remain available |
| Home Assistant or Wi-Fi offline | Original hall button remains available |
| Relay offline | Original hall button remains available |
| Camera or recognizer offline | Calling still works; floor telemetry becomes unavailable |
| MQTT offline | Original hall button remains available; smart control and display telemetry pause |

## Local-first design

Normal operation stays on the local network. MQTT decouples producers and
consumers: the recognizer does not need to know about the display, and the
display does not need camera access. Home Assistant supplies other data such as
weather, air quality, air-raid state, grid availability, inverter and battery
telemetry through the same interface.

## Public configuration boundary

The repository may include example topic names, services and entity mappings,
but never real Wi-Fi credentials, MQTT passwords, camera URLs, access tokens,
private IP addresses, apartment numbers or identifying full-cabin recordings.
