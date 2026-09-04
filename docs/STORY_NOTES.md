# Call14 story notes

This file contains confirmed source material for the public English case study.
It is intentionally factual and may be rewritten later for narrative flow.

## Confirmed background

- The idea first appeared during conversations with a friend and neighbor about
  useful smart-home improvements for their apartment building.
- The builder lives with his family on the 14th floor in Kyiv.
- Elevator waiting time varies from immediately available to five minutes or
  more.
- Air-raid alerts and the need to get children ready and reach shelter during
  missile attacks became a strong motivation for completing the remote-call
  feature.
- The builder is an entrepreneur and hobbyist, not a professional programmer
  or electronics engineer. This was his first project of this kind.

## Stage 1 — the Lift Button

- The first user interface was a small round Xiaomi wireless smart button.
- The family called it “Lift Button” / “Кнопка лифт”; that name remained even
  after the project expanded.
- An ESP8266 relay module was connected in parallel with the existing hall-call
  button.
- The relay does not access elevator motion or safety control. It only closes
  the same button contacts for a short pulse.
- Opening the original call-button panel required an unusual security bit; in
  practice, ordinary needle-nose pliers solved the problem.
- The relay module required UART flashing and hardware GPIO-to-relay jumpers.
- The selected module is marked `XY-WF36V` and contains an ESP8285H16 module.
- It was selected for its nominal 5–30 V DC input range, based on an expectation
  that the hall button would provide 12–24 V DC.
- Measurement showed approximately 24 V AC instead. The relay therefore could
  not be powered directly from the button circuit.
- Roughly 10 metres of low-voltage cable was run from an existing access-control
  power supply on the floor.
- The original Chinese relay application could have provided a simpler phone-
  only version, but custom firmware was chosen for Home Assistant integration.
- GPIO0 was held to ground during reset/power-up to enter the ESP8266 serial
  bootloader for the first flash.
- The successful first experience was memorable: press the button inside the
  apartment, finish getting ready, walk into the hallway, and have the doors
  open immediately — “like a movie secret agent.”

## Stage 2 — finding the elevator

- Remote calling solved only half of the problem: the family still could not
  tell whether the elevator was already nearby or several floors away.
- A direct connection to the UEL elevator controller and its X7 dispatcher
  interface was investigated.
- The elevator service company was contacted. The builder explained that he was
  a self-taught entrepreneur who had created a hobby device and wanted to
  connect it to the elevator control board.
- The company was understandably surprised and declined the request carefully.
- Direct controller integration was abandoned. A read-only camera-based design
  was chosen instead.

## Stage 3 — camera recognition

- An authorized cabin camera already had a view of the red LED floor indicator.
- Initial approaches included generic OCR, ssocr, and template matching.
- Real-world failures included reflections, changing exposure, passengers,
  open-door lighting, incomplete multiplexed LED segments, and direction-arrow
  flicker.
- Several recognizer generations were tested. The successful v5 design combines
  a seven-segment decoder with a directional template fallback and a physical
  state machine.
- It rejects impossible floor jumps, preserves the last reliable reading during
  poor frames, debounces missing arrows, and permits careful resynchronization
  only after independent methods agree at a stop.

## Stage 4 — the display

- An inexpensive JUZIPi SD_PRO smart weather clock was purchased for about 200
  UAH. The confirmed promotional total was 224.26 UAH including delivery
  (roughly USD 5 at the time); the listing price shown was 279.79 UAH.
- Its actual hardware is ESP8266 / ESP-12F with an ST7789 240×240 display.
- The photographed PCB is marked `2025/11/10 V1.1`; the display flex is marked
  approximately `WA548C049I-10Z`.
- The original firmware was replaced with a custom PlatformIO/Arduino firmware.
- Development required repeated enclosure disassembly, UART soldering, boot-mode
  recovery, recovery builds, and restoration of OTA access.
- A physical call button was added to a safe free GPIO.
- The `O` pad in the rear `G V O` group was continuity-tested and confirmed as
  GPIO4. The normally-open tactile switch shorts `O` to `G` and uses
  `INPUT_PULLUP` in firmware.
- The enclosure was drilled at the top and the spare tactile switch was glued
  in place. A capacitive touch sensor remains a possible no-drill alternative.
- Holding GPIO4 low during startup activates a display-safe recovery mode while
  network and web update services remain available.
- A short press calls the elevator from the normal screen; when the elevator
  view is already visible, it advances the screen. A long press cycles display
  brightness.
- The display normally shows time, weather, air quality, elevator floor, and
  home status. During a call it switches to a large elevator interface.

## Ukrainian context

- The display also reports air-raid alerts.
- Home Assistant already uses an air-raid integration that controls TV ambient
  lighting.
- Automatic elevator calls tied to alerts were considered but intentionally
  left manual because alerts occur frequently and a call should represent a
  real intention to leave.
- Russian attacks on Ukrainian energy infrastructure make grid availability,
  inverter operation, and battery charge useful everyday information.
- The device evolved from an elevator accessory into an always-on home status
  terminal.

## Editorial principles

- Keep the story personal and honest; do not pretend it was fully designed in
  advance.
- Describe failed attempts and reversals alongside successful decisions.
- “Not a professional programmer or engineer” is context, not an apology.
- Never imply that Call14 is an emergency or life-safety system.
- Redact credentials, private network details, camera addresses, exact apartment
  information, and identifiable faces before publication.
