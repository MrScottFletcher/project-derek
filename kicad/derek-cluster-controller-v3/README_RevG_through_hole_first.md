# Derek Cluster Controller Rev G — Through-Hole First

This revision simplifies assembly and minimizes SMT components.

## Main changes
- SW1 is a plain 4-position DIP switch connected directly to ESP32 GPIO4–GPIO7.
- Four 10 kΩ through-hole pull-up resistors keep the ID stable; switch ON connects the GPIO to GND.
- Status LEDs and their 1 kΩ resistors are through-hole.
- WS2812 level shifting uses a socketable SN74AHCT125N DIP-14 plus a 330 Ω through-hole resistor.
- Large/local capacitors remain through-hole: 4700 µF at input, 1000 µF near logic/modules, and 100 µF per pair of RJ45 ports.
- Plug-in modules and connectors remain hand-assembled.
- Q1 is the only intentionally retained SMT part because a low-resistance D2PAK MOSFET is more suitable for the high-current reverse-polarity path than a small through-hole device. It can be DNP if reverse-polarity protection is handled externally.

## Important limitations
- This remains a prototype design package. Run ERC and DRC in KiCad.
- Refill copper zones after opening the PCB.
- The updated THT footprints change pad positions, so inspect and finish routing around U5, R3, R20-R22, R30-R33, and D2-D4.
- Verify the exact ESP32, RJ45, DFPlayer, OLED, and PCA9685 footprints with a 1:1 print before PCBWay.
- Confirm the power terminal, fuse holder, board copper, cabling, and connectors are genuinely rated for the expected current.

## Assembly strategy
PCBWay SMT: Q1 only (optional).
User assembly: all modules, connectors, resistors, LEDs, capacitors, headers, fuse holder, and DIP switch.
