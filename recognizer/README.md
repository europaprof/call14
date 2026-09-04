# Call14 hybrid floor recognizer v5

This service turns a fixed camera view of a seven-segment elevator indicator
into local MQTT state for Home Assistant. It combines calibrated segment masks,
direction-specific templates, temporal voting, arrow hysteresis and a physical
floor state machine.

The included model and templates are evidence from the Call14 installation.
They demonstrate the method but will not align with another camera. Recalibrate
the crop, segment masks, arrow boxes, thresholds and valid-floor sequence for
each installation.

## Runtime requirements

- Linux with Python 3
- Pillow
- FFmpeg with access to the authorized local camera stream
- `mosquitto_pub`
- an MQTT broker reachable from the service

CPU decoding is the portable default. Set `LIFT_HWACCEL=vaapi` only after
confirming that FFmpeg can use the configured render device.

## Installation outline

1. Copy this directory to `/opt/call14/recognizer`.
2. Install the package in `requirements.txt` and the system FFmpeg/Mosquitto
   clients.
3. Copy `call14-recognizer.env.example` outside the repository to
   `/etc/call14-recognizer.env`; set the private RTSP and MQTT values there.
4. Calibrate a model for the actual camera. See `CODEX_SETUP_PROMPT.md` for a
   guided adaptation workflow.
5. Install `lift-recognizer.service.example` as a systemd unit, adjust its user
   and paths, then enable it.
6. Run first with a separate MQTT prefix and compare a complete recorded ride
   against ground truth before switching any dashboard to it.

## Published state

The service publishes a retained JSON state and simpler floor/direction/display
topics. Home Assistant MQTT discovery creates sensors for floor, state,
direction, confidence, source, availability and fault condition.

## Privacy and safety

Process only a legitimate, authorized camera stream. Prefer a crop containing
only the indicator, do not publish identifiable cabin footage, and keep camera
credentials outside the repository. This software observes an indicator; it
must not be used as an elevator safety or evacuation system.
