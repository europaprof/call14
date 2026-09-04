# Call14 Reddit launch kit

Repository: https://github.com/europaprof/call14

## Recommended first post

Best fit: `r/homeassistant`

### Title

I built a $20 smart elevator companion that calls the elevator and tracks its floor with computer vision

### Post

I live on the 14th floor in Kyiv. Waiting for the elevator can take five
minutes, but sometimes it is already outside the door. During air alerts, when
I am getting the children ready to go to shelter, even a small head start can
matter. I wanted to call the elevator while we were still putting on our shoes
and to know where the cabin actually was.

I am an entrepreneur, not a programmer or an electronics engineer. This is my
first project of this kind, built as a hobby over roughly six months.

The first part was simple in principle: an ESP8266 relay is wired in parallel
with the existing hall-call button, so it only reproduces an ordinary button
press. It does not connect to the elevator controller or any safety system. I
can trigger it from Home Assistant, a Xiaomi wireless button, my phone, or the
button I added to a tiny ESP8266 clock.

The clock was a cheap AliExpress weather clock. I replaced its firmware and
built a new interface showing time, weather, energy flow, power-outage state,
air-alert status, elevator floor and direction. It looks much more finished
than I expected from something repeatedly opened, soldered and reflashed on my
desk.

Tracking the elevator was the difficult part. I first tried ordinary OCR and
full-image templates against a camera view of the cabin display. They failed
whenever the doors opened, lighting changed, passengers entered, or a courier's
glossy helmet changed the reflections. I nearly abandoned computer vision and
asked the elevator company about reading their controller protocol. They were
very surprised and eventually found a polite way to say no.

The breakthrough was realizing that I did not need to recognize the whole
image. The indicator is a seven-segment display, so I only needed to measure
whether a few fixed segments were illuminated. The current v5 combines those
segment probes with direction-specific templates, temporal voting, a movement
state machine and physically valid floor transitions. Bad frames preserve the
last confirmed state instead of producing an attractive but impossible guess.

In daily use, every result I have checked since deploying v5 has been correct.
That is 100% observed accuracy for this installation so far, not a universal
benchmark. Day/night changes, open doors, crowded rides and strong reflections
have not caused a known error in the current version.

The entire system runs locally through Home Assistant and MQTT. My out-of-pocket
hardware cost was about $20 because the server is an old Mac I already owned.

I published the full story, sanitized source code, ESPHome relay configuration,
display firmware, recognizer v5, calibration files, wiring notes, photos and
demos here:

https://github.com/europaprof/call14

I would genuinely appreciate technical feedback, especially on how you would
make the recognizer easier to calibrate for other seven-segment displays.

Safety note: Call14 never controls movement, doors, brakes or safety circuits.
It only duplicates a normal hall-button press and observes the existing cabin
display. It is not a life-safety system, and official emergency guidance always
takes priority.

## Suggested first comment

A few details that did not fit cleanly in the post:

- Relay: XY-WF36V / ESP8265, reflashed with ESPHome. Mine could not use the
  elevator button supply because it turned out to be 24 V AC, not DC, so I had
  to run about 10 m of separate low-voltage power cable.
- Display: ESP-12F clock board with a 240×320 TFT. The case now has one tactile
  button connected between a free GPIO and ground. A capacitive touch button
  would avoid drilling the enclosure.
- Recognizer: Python + OpenCV + FFmpeg + MQTT. The configured floor sequence is
  `1, 4, 5, …, 16`, matching the real building rather than assuming every
  integer is valid.
- Privacy: processing is local, credentials and private addresses are excluded,
  and no passenger dataset is published.
- Licenses: MIT for code; CC BY 4.0 for my original documentation and media.

The failure history and why v5 works are documented separately in the repo, not
hidden behind a polished final result.

## Alternative titles

### For r/homeassistant

I turned a $6 ESP8266 clock into a Home Assistant elevator display and call button

### For r/esp8266

Two reflashed ESP8266 devices became a local smart elevator call and tracking system

### For r/diyelectronics

My first electronics project: a $20 elevator companion with a custom display and seven-segment vision

### For r/computervision

OCR kept failing on this elevator display, so I recognized the illuminated segments instead

## Posting order

1. Publish the main story to `r/homeassistant` with the finished-device image or
   short muted demo.
2. Answer questions for at least a day before adapting the post elsewhere.
3. For `r/computervision`, lead with the segment-mask overlay and focus on the
   failure modes, state machine and validation rides.
4. For `r/esp8266`, lead with the clock teardown and UART recovery story.
5. Never paste the identical text into several communities at once; tailor it
   to each audience and check that community's current rules immediately before
   posting.

## Short reply to “Is this safe?”

The relay is only a dry contact wired in parallel with the existing hall-call
button, equivalent to pressing that button. The vision side is read-only. It
does not connect to or command the elevator controller, doors, drive, brakes or
safety circuits. Installation still needs building approval and qualified
electrical review where required.

