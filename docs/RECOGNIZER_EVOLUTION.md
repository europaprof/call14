# From OCR to a reliable seven-segment reader

This document records how the Call14 floor recognizer evolved and why the final
hybrid design works better than the earlier approaches.

## Constraints

The input is a perspective view of a glossy red, multiplexed LED indicator from
an authorized local camera. Recognition must tolerate door light, day/night
exposure, passengers and moving reflections, while never treating an uncertain
frame as permission to invent a floor.

The service processes a fixed crop locally at 4 fps and publishes only state to
MQTT. The public project does not contain camera URLs, credentials, addresses,
or full-cabin recordings.

## Evolution

### 1. Conventional OCR

OCR was the natural first experiment, but the source is not printed text. The
small slanted LED strokes, multiplexing and glare created unstable characters.

### 2. Whole-digit templates

Normalized image templates improved the result, especially once templates were
separated by direction. They still compared too much irrelevant image area, so
lighting changes and reflections could dominate the distance metric.

### 3. Temporal and physical constraints

Voting and a valid-floor state machine rejected many impossible jumps. A
time-based dead-reckoning fallback was also tried, then removed: one false arrow
could make it confidently manufacture a sequence of floors. Camera evidence is
now authoritative; uncertainty retains the last proven value.

### 4. Point masks for the seven segments

After repeated real-world failures, the camera approach was close to being
abandoned in favour of requesting permission for a direct connection to the
elevator controller. Reframing the problem avoided that unsafe and difficult
path.

The key product insight came from the project owner: identify the seven physical
segments at stable points rather than recognize the complete photographed digit.
The implementation measures masks for `a–g` plus the tens digit and converts the
active set into an exact seven-segment code.

### 5. Production hybrid

The current implementation combines:

- exact segment decoding as the primary source;
- direction-specific normalized templates as fallback and independent evidence;
- valid-floor and monotonic-motion constraints;
- candidate voting;
- arrow hysteresis for LED scan gaps;
- strict two-decoder stationary re-synchronization;
- fault/display-off detection and retained last-known-good state;
- MQTT discovery for Home Assistant.

The state machine knows the actual served sequence:

```text
1 ↔ 4 ↔ 5 ↔ 6 ↔ 7 ↔ 8 ↔ 9 ↔ 10 ↔ 11 ↔ 12 ↔ 13 ↔ 14 ↔ 15 ↔ 16
```

This is why a clean-looking but impossible digit does not overwrite the current
floor.

## Validation

Development used recorded end-to-end journeys, frame extraction and contact
sheets. Important test sequences included:

- `14 → 1 → 16 → 14`
- `1 → 14 → 1 → 16`

The recordings exposed failures that isolated screenshots did not: arrow scan
gaps, a digit changing between adjacent frames, lighting transitions at open
doors, crowds, and reflections. Candidate releases ran in shadow mode before
replacing the previous MQTT producer.

The public contact sheets in `media/recognizer/` are cropped to the indicator.
They demonstrate difficult input without publishing identifiable cabin footage.
One full empty-cabin frame is included to show how small the indicator is in the
original camera image.

At the time of publication, all v5 results manually checked against the real
elevator have been correct, giving 100% observed accuracy so far. This describes
the observed production run on this installation; it is not a universal accuracy
guarantee or a substitute for a separately labelled benchmark dataset.

## Authorship and collaboration

Call14 was an iterative human–AI collaboration. The project owner defined the
real problem, proposed the segment-point strategy, installed the hardware,
performed test rides, supplied ground truth, and rejected behaviour that did not
match the elevator. The implementation and successive algorithm revisions were
developed with an AI coding assistant under that direction and real-world
validation.

That distinction matters: the code could optimize what was measurable, but the
owner supplied the physical insight and the repeated field testing that made the
system useful.
