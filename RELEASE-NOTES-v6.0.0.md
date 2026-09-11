# Elevator Floor Recognizer v6.0.0

This release fixes the intermittent degradation seen after the system had been
working correctly for several days.

## Root cause

The RTSP image/camera crop moved vertically by approximately 4-5 pixels relative
to the calibration recording. The previous recognizer used fixed pixel masks,
so this small shift made the masks sample LED edges and gaps instead of the
seven physical bars. It caused errors such as 7/1, 9/4 and a tracker that could
appear stuck on an old floor. A service restart did not address the geometry.

## Resolution

v6 aligns every incoming display crop to its calibration reference using image
gradients on stationary metal outside the digits and arrows. Unsafe alignment is
rejected instead of guessed. Three independent classical decoders then vote
under the physical constraints of the elevator.

Additional reliability work includes improved arrow detection, LED multiplexing
hold, persisted confirmed state, controlled stationary recovery, RTSP stall and
buffer watchdogs, MQTT timeouts, reconnect cleanup, and per-camera diagnostics.

## Upgrade warning

The bundled model documents one tested installation. Every other camera requires
its own crop, templates, segment masks and alignment reference. Do not reuse an
alignment model for a camera with different framing.

Full technical details and measured validation results are in `CHANGELOG.md` and
`docs/ALIGNMENT.md`.
