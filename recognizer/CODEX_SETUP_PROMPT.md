# Codex prompt: adapt Call14 recognition to my elevator

Copy the prompt below into Codex while working in a clone of this repository.
Attach representative authorized recordings or still images from your own
indicator. Do not place camera passwords or identifiable passenger footage in
the repository.

---

I want to adapt the Call14 hybrid seven-segment elevator recognizer in
`recognizer/` to my own authorized camera and indicator.

Work locally and preserve the safety boundary: the result may observe the
indicator and publish MQTT telemetry, but it must not connect to or control the
elevator controller, doors, movement, brakes, or safety circuits.

Please do the following:

1. Inspect `recognizer/lift-recognizer-v5.py`, `docs/RECOGNIZER_EVOLUTION.md`,
   and the example environment file. Explain which parameters are specific to
   the Call14 installation.
2. Inspect my supplied sample video/images. First make privacy-safe crops that
   contain only the floor indicator and arrows. Never commit RTSP URLs,
   credentials, faces, addresses, or full unredacted recordings.
3. Determine the FFmpeg crop, digit box, up/down arrow boxes, actual served-floor
   sequence, camera frame rate and display orientation. Put private stream data
   only in an environment file outside Git.
4. Build a calibration contact sheet covering every served digit in idle, up
   and down states where possible. Account for multiplexed LEDs: a single frame
   may contain incomplete segments.
5. Calibrate small evidence masks inside segments `a` through `g` and the tens
   digit. Visualize the masks on a real cropped frame so I can approve their
   locations.
6. Generate direction-specific fallback templates and thresholds. Keep segment
   decoding primary when its exact code is clean; use template matching only as
   independent fallback evidence.
7. Configure physical validation so a reflection cannot produce an impossible
   jump. Encode my actual floor order, including any skipped labels such as a
   jump from 1 to 4.
8. Preserve the last confirmed floor on unreadable frames. Do not add
   time-based dead reckoning that invents floors after uncertain arrow evidence.
9. Add arrow hysteresis for LED scan gaps and strict re-synchronization that
   requires sustained agreement between the segment and unrestricted template
   decoders while stopped.
10. Create an offline replay test from at least two complete labelled rides.
    Report every confirmed-floor transition and compare it with ground truth.
11. Run the candidate as a shadow service with a separate MQTT prefix. Do not
    replace an existing working service until replay and live observations pass.
12. Produce generic installation files and documentation. Before finishing,
    scan the complete diff for passwords, tokens, private IP addresses, camera
    URLs, personal data, and identifiable images.

Ask me only for facts that cannot be inferred safely from the supplied frames,
such as the true floor at an ambiguous timestamp or the real served-floor list.
Show evidence for each calibration decision and keep a reversible backup of any
existing service configuration.

---

The prompt intentionally asks Codex to calibrate rather than blindly reuse the
included model. Camera position, lens, display geometry and lighting are unique
to each installation.
