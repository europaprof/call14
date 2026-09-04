# Wiring and verified pinouts

These notes describe the photographed Call14 hardware. Board revisions can
change without a new product name, so verify continuity and voltage on your own
unit before connecting it.

## Hall-call relay

The XY-WF36V relay contact is wired across the same two terminals that close
when the original hall-call button is pressed:

```text
Original hall-call circuit A ──┬──── original button ────┬── circuit B
                              └──── relay COM → NO ─────┘
```

Use the normally-open dry contact. Do not inject voltage into the button circuit
and do not connect to the elevator controller bus.

The installed button circuit measured approximately 24 V AC. The relay module
expects DC power, so Call14 uses a separate low-voltage DC supply rather than
trying to power it from the hall button.

### Relay UART/programming header

The XY-WF5V reference diagram matched the programming-pad order on the tested
XY-WF36V:

```text
GND · TX · RX · GPIO0 · RST · 3V3
```

Connect adapter TX to board RX, adapter RX to board TX, and common GND. Use a
regulated **3.3 V** UART interface. Hold GPIO0 to GND during reset/power-up for
the serial bootloader, then release it for normal boot. Disconnect the installed
power source and elevator wiring while bench-flashing.

The ESPHome configuration uses:

| Function | GPIO |
|---|---:|
| Relay | 4 |
| Green LED | 13 |
| Board button | 12, active low |

## Clock/display

Verified on the photographed ESP-12F board marked `2025/11/10 V1.1`:

| Function | GPIO |
|---|---:|
| ST7789 SCLK | 14 |
| ST7789 MOSI | 13 |
| ST7789 CS | 15 |
| ST7789 DC | 0 |
| ST7789 reset | 2 |
| Backlight | 5, active low |
| Added call button (`O` to `G`) | 4, `INPUT_PULLUP` |

The added normally-open tactile switch simply closes GPIO4 (`O`) to ground
(`G`). Do not use the middle `V` pad for this button.

### Clock UART recovery

Use only a 3.3 V USB-to-UART adapter and cross TX/RX. GPIO0 must be low at reset
to enter the ESP8266 ROM bootloader. ESP8266 boot-strap pins GPIO0 and GPIO2 must
normally be high and GPIO15 low; accidental loading of these display pins can
prevent boot.

The photos document the successful recovery setup, but wire colors are not a
pinout. Confirm every connection with the PCB/module markings and a multimeter.
Keeping temporary, insulated UART leads available during the first experimental
flash is much easier than repeatedly soldering directly to the ESP module.

## Electrical and building safety

- Obtain any required building/owner authorization.
- Measure both voltage and AC/DC type before selecting a module.
- Never work on energized wiring.
- Never connect Call14 to motion, brake, door, interlock or safety circuits.
- Keep the original hall button functional and independent.
- Treat Call14 as a convenience accessory, never as emergency equipment.
