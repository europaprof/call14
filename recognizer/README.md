# Call14 hybrid floor recognizer v6

This service turns a fixed camera view of a seven-segment elevator indicator
into local MQTT state for Home Assistant. It first registers every frame to a
camera-specific reference, then combines calibrated segment masks,
direction-specific templates, temporal voting, arrow hysteresis and a physical
floor state machine.

## Why v6 exists

The previous fixed-pixel implementation worked well and then appeared to degrade
or get stuck. A labelled-frame audit found that the RTSP picture had shifted
vertically by about 4-5 pixels relative to calibration. Before alignment only
33 of 145 sampled frames decoded correctly; compensating the measured shift
raised that to 141 of 145. This was camera/crop movement, not accumulated Python
state or changing cabin lighting.

`lift_alignment.py` now estimates a small translation from gradients on the
stationary metal beside the display. Digits and arrows are excluded from the
alignment evidence. Frames below the safe correlation threshold are reported as
`unreadable` and cannot change the confirmed floor.

The included model and templates are evidence from the Call14 installation.
They demonstrate the method but will not align with another camera. Recalibrate
the crop, segment masks, arrow boxes, thresholds and valid-floor sequence for
each installation.

## Runtime requirements

- Linux with Python 3
- Pillow
- NumPy
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
4. Calibrate segment/template models and an alignment reference for the actual
   camera. See `CODEX_SETUP_PROMPT.md` for a guided adaptation workflow.
5. Install `lift-recognizer.service.example` as a systemd unit, adjust its user
   and paths, then enable it.
6. Run first with a separate MQTT prefix and compare a complete recorded ride
   against ground truth before switching any dashboard to it.

## Published state

The service publishes a retained JSON state and simpler floor/direction/display
topics. Home Assistant MQTT discovery creates sensors for floor, state,
direction, confidence, source, availability and fault condition.

The JSON diagnostics also contain `alignment_offset`, `alignment_quality` and
the results of all three independent decoders. Normal alignment quality for the
included camera is above 0.60; frames below 0.45 fail closed.

## Included production evidence

The bundled Call14 camera calibration passed:

- a fresh full-route replay with 361 correct and 0 wrong accepted results;
- an independent holdout ride with 236 correct and 0 wrong accepted results;
- recovery from a deliberately wrong persisted floor;
- loss-of-alignment and stalled-RTSP watchdog tests.

These results describe this installation and test material, not a universal
accuracy guarantee. Every different camera still requires its own calibration.

## Privacy and safety

Process only a legitimate, authorized camera stream. Prefer a crop containing
only the indicator, do not publish identifiable cabin footage, and keep camera
credentials outside the repository. This software observes an indicator; it
must not be used as an elevator safety or evacuation system.
