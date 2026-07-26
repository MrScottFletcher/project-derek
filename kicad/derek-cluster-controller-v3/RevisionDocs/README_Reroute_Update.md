# Rev F Adafruit 815 PCA9685 Reroute Update

This package removes stale PCA9685 servo routing left over from the earlier generic J26 footprint and adds new initial routing from the Adafruit Product 815 J26 footprint to the eight RJ45 Derek ports.

## What changed
- Removed old routed segments for servo nets D1_PAN/D1_LIFT through D8_PAN/D8_LIFT.
- Added new In3.Cu signal routes from J26 Adafruit 815 servo output pads to RJ45 pins 5 and 6.
- Kept the schematic, BOMs, and assembly-oriented structure from Rev F.

## Still required
- Open KiCad PCB Editor and run DRC.
- Visually inspect all new routes.
- Tune routes for clearance/appearance if desired.
- Confirm J26 with a 1:1 print and the real Adafruit 815 board.
- Confirm RJ45 footprint with the actual 90-degree shielded jacks.
