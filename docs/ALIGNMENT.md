# Camera jitter and image alignment

## The failure mode

The recognizer originally assumed that the display always occupied the same
pixels. In production the complete crop shifted down by roughly 4-5 pixels.
That is enough for narrow seven-segment masks to sample an unlit edge or the
next bar. Restarting the service could appear to help, but did not correct the
underlying geometry.

## Version 6 approach

`PanelAlignment` searches a small translation window (`dx=-3..3`,
`dy=-8..10`) and compares image gradients only on stationary metal to the left
and right of the display. Digits and arrows do not influence registration.
The winning offset transforms the crop back into calibration coordinates.

When normalized correlation is below `0.45`, the frame is rejected and MQTT
reports `state=unreadable`. The recognizer keeps the last confirmed floor but
does not guess a new one.

## Per-camera calibration

`recognizer/panel-alignment.json` belongs to the bundled Call14 passenger camera
and its exact 120x95 crop. For another camera, capture a sharp stationary crop
with the same dimensions and build its reference:

```bash
python3 recognizer/tools/build_alignment.py display-crop.jpg recognizer/my-lift-alignment.json
```

Segment/template models and ROI coordinates also remain camera-specific.

Confirm the live MQTT diagnostics before acceptance:

- `alignment_quality` should normally be above `0.60`;
- `alignment_offset` should be stable or change only by a few pixels;
- sustained low quality means the crop, resolution, angle or reference no longer
  matches and must be recalibrated.
