# Call14 media checklist

Place the first material batch in `media/photos/` and `media/video/`.

## First batch

- [x] `11-call14-finished-main-screen.jpg` — finished device, normal UI visible
- [ ] `02-main-clock-screen.jpg` — normal clock/weather screen
- [ ] `03-physical-call-button.jpg` — finger pressing the added button
- [x] `06-xiaomi-lift-button.jpg` — original round Xiaomi button
- [x] `03-xy-wf36v-front.jpg` — ESP8285 relay board
- [x] `05-xy-wf36v-uart-flashing.jpg` — UART flashing connection
- [x] `07-clock-purchase-224uah.jpg` — order and confirmed promotional total
- [x] `08-sd-pro-board-back.jpg` — rear PCB with G/V/O pads
- [x] `09-sd-pro-board-esp12f-display.jpg` — ESP-12F and display
- [x] `10-sd-pro-uart-workbench.jpg` — UART/programmer workbench
- [x] Home Assistant elevator dashboard and live camera view
- [x] Home Assistant MQTT device and diagnostic entities
- [x] Full empty-cabin source frame with floor display visible
- [x] `call14-clock-demo.mp4` — physical press and real-time clock operation,
      with the original audio removed
- [x] `call14-clock-demo.gif` — accelerated silent README preview
- [x] `call14-power-outage-demo.mp4` — edited real-time power-outage response,
      with the waiting pause shortened and original audio removed
- [x] `call14-power-outage-demo.gif` — accelerated silent outage preview
- [ ] `01-call14-full-demo.mp4` — future press, track, leave apartment and
      elevator-arrival sequence

## Capture rules

- Prefer original files; do not send through a messenger that destroys quality.
- Capture the main demo horizontally at 1080p or 4K and 30 fps.
- Avoid visible apartment numbers, addresses, credentials, camera URLs, tokens,
  notifications, and identifiable faces.
- Do not reopen or rewire a working powered device only for a photograph.
- Do not photograph energized mains wiring.

## Received: relay chapter

- [x] Original hall-call button
- [x] Parallel connection behind the button
- [x] XY-WF36V front and rear
- [x] UART flashing connection
- [x] Original Xiaomi smart button
- [ ] Clear photo of the installed 10 m low-voltage supply cable or its safe
      endpoint, only if it can be photographed without exposing energized wiring
- [ ] Optional screenshot of the Home Assistant automation for the Xiaomi button

`reference-xy-wf5v-pinout.jpg` is reference-only. It visibly shows a different
model marking (`XY-WF5V`) from the real `XY-WF36V` board and must not be
published as the confirmed pinout until compatibility is verified.

The builder confirms that the XY-WF5V and XY-WF36V programming pads share the
same pin order and that the reference image was used successfully. We should
still caption it explicitly as an internet-sourced reference illustration, not
as a photograph of the installed board, and verify reuse rights before public
publication.

## Received: display chapter

- [x] Purchase screenshot and price
- [x] Display and PCB disassembled
- [x] ESP-12F module and display-flex marking
- [x] UART/programmer setup
- [x] Finished device on the shelf
- [x] Exterior top, side, rear, and bottom views
- [x] Internal tactile-button wiring
- [x] Dedicated elevator-mode screen in `IDLE`
- [ ] Finger visibly pressing the physical button
- [ ] Earlier UI versions, if screenshots or photographs exist

## Required: firmware UI gallery

The remaining states will be illustrated directly from the firmware layout.
The builder does not need to photograph each state manually.

- [x] Home screen
- [x] Elevator — idle
- [ ] Elevator — calling
- [ ] Elevator — moving up
- [ ] Elevator — moving down
- [ ] Elevator — arrived
- [x] Energy-flow screen
- [x] Inverter/power-outage screen
- [ ] Charger screen
- [ ] Full-screen air-raid alert
- [x] Persistent air-raid indicator on the home screen
- [ ] All-clear screen
- [ ] Motion notification, if currently used
- [ ] Boot or diagnostic screen
- [ ] Web control/status page screenshot, with private values redacted

Optional but valuable: photographs or screenshots of early interface versions
that show how typography, colors, animation, and information hierarchy evolved.
