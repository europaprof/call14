# Changelog

## 6.0.0 - 2026-09-12

Version 6 fixes the long-running degradation that looked like recognition was
"getting stuck" after several days. The camera image had moved vertically by
about 4-5 pixels relative to the calibration recordings, so fixed segment masks
were sampling the gaps between LED bars.

### Added

- Per-frame translation alignment against stationary metal outside the display.
- Alignment offset and quality in MQTT diagnostics.
- Fail-closed `unreadable` state when registration quality is unsafe.
- Independent green/blue, red-channel and physical-geometry seven-segment decoders.
- Controlled stationary recovery after three seconds of exact agreement between
  two physical decoders; this repairs a wrong persisted floor without accepting glare.
- Persistent confirmed floor across service restarts.
- RTSP no-frame watchdog, bounded JPEG buffer and automatic FFmpeg reconnect.
- MQTT publish timeout and retry-safe state handling.
- Multi-instance systemd state directory and NumPy dependency.

### Changed

- Direction arrows use small illuminated cores with adjacent background subtraction.
- Arrow state survives short LED multiplexing gaps.
- Incomplete red segment codes cannot override an exact physical geometry code.
- Invalid/alignment-lost frames clear pending confirmations.
- Templates are direction-aware and the tested Call14 calibration is bundled.

### Validation

- Original labelled route: 141/145 directly decoded frames after alignment
  (33/145 without alignment).
- Fresh full route replay: 361 correct, 0 wrong.
- Independent holdout ride: 236 correct, 0 wrong.
- Recovery from a deliberately wrong saved floor: passed.
- Alignment loss, stalled RTSP source and reconnect safety tests: passed.

This is a major release because every frame now passes through spatial
registration before recognition. Existing calibrations should not be upgraded
blindly; create an alignment reference for each camera.
