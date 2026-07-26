
# Derek Cluster Controller Rev F - Assembly-Oriented Package

This revision is intended for the practical prototype workflow:

- PCBWay performs SMT assembly for simple, standard SMT parts only.
- You hand-install through-hole connectors, sockets, and plug-in modules.
- The board is not intended for full turnkey PCBWay assembly of Amazon/AliExpress modules.

## Assembly split

### PCBWay SMT assembly
Use `PCBWay_SMT_BOM_DRAFT.csv` as the starting point.

PCBWay should install parts such as:

- 0805 resistors
- 0805 capacitors
- 0805 status LEDs
- SOIC-14 74AHCT125 / 74HCT125 level shifter
- optional SMT MOSFET reverse-protection parts if you lock the exact footprint and MPN

You will still need to generate the official PCBWay BOM and pick-and-place/CPL files from KiCad after final footprints and routing are complete.

### Hand assembly
Use `Hand_Assembly_BOM.csv`.

You hand-install:

- Hosyond ESP32-S3 N16R8 development board socket/module
- Adafruit Product 815 PCA9685 servo driver module
- WWZMDiB DFPlayer Mini modules
- 0.96 inch SSD1306 OLED module
- RJ45 right-angle jacks
- fuse holder
- screw terminal / power connector
- large electrolytic capacitors
- PIR connectors
- jumpers and test headers

## Footprint strategy

For Rev F, footprints are intentionally practical rather than over-modeled:

- Plug-in modules use through-hole header/socket footprints and silkscreen outlines.
- PCBWay SMT parts use standard KiCad footprints where practical.
- RJ45, ESP32, PCA9685, DFPlayer, and OLED must still be printed 1:1 and physically test-fit before fabrication.

## Minimum checks before ordering

1. Open the schematic and run ERC.
2. Assign/verify footprints for every symbol.
3. Open PCB Editor and run Update PCB from Schematic.
4. Confirm all eight RJ45 connectors are aligned on one edge and accessible.
5. Confirm DFPlayer microSD slots face an accessible edge.
6. Confirm ESP32 USB-C ports are accessible.
7. Confirm ESP32 antenna is at the board edge with no copper under the antenna on any layer.
8. Route high-current 5V first using the 5V plane/pours.
9. Run DRC.
10. Print the PCB 1:1 and physically test-fit every module and through-hole component.
11. Only then generate Gerbers, drill files, BOM, and CPL files for PCBWay.

## Power-routing design intent

The board target is 5V at up to about 15A total available current for servos/LEDs. Do not route that through thin traces.

Recommended stackup:

- L1: components and signal routing
- L2: continuous GND plane
- L3: protected 5V plane / high-current pours
- L4: signals
- L5: secondary GND/stitch plane
- L6: signals / low-current power

Power path:

5V input -> fuse -> reverse-protection -> 4700uF bulk cap -> protected 5V plane -> RJ45 power zones.

Each RJ45 port uses:

- Pin 1: +5V protected
- Pin 2: +5V protected
- Pin 3: GND
- Pin 4: GND
- Pin 5: Pan PWM
- Pin 6: Lift PWM
- Pin 7: WS2812 Data In
- Pin 8: WS2812 Data Out

Silkscreen warning required near RJ45 row:

NOT ETHERNET - 5V POWER PRESENT

## Files added in Rev F

- `PCBWay_SMT_BOM_DRAFT.csv`
- `Hand_Assembly_BOM.csv`
- `README_RevF_assembly_oriented.md`

This is still a prototype package. It is closer to the practical manufacturing workflow, but it is not fabrication-ready until ERC/DRC, footprint test-fit, routing, and final PCBWay exports are completed.
