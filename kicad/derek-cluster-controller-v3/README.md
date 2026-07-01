# Derek Cluster Controller Rev D - 220 x 220 mm, 6-layer, clean placement

This is a fresh KiCad starter derived from the Rev C concept, with the focus requested:

1. More component-specific footprints rather than generic blocks.
2. Power distribution intended to support portions of a 15A total 5V budget to the RJ45 Derek connectors.
3. No deliberate component overlaps.
4. Initial trace routing and power-plane structure.

## Important limitations
This is still a starter project, not a finished PCB. It does not replace a real KiCad ERC/DRC workflow. The ESP32-S3, Adafruit PCA9685, DFPlayer Mini, OLED, and especially RJ45 clone footprints MUST be verified with a 1:1 printout against the exact physical parts before sending to PCBWay.

## Board
- Size: 220 mm x 220 mm
- 6 layers:
  - F.Cu: components / primary signals / local high-current bus
  - In1.Cu: GND plane
  - In2.Cu: protected 5V plane
  - In3.Cu: signal layer
  - In4.Cu: secondary GND/stitch plane
  - B.Cu: secondary signals

## Power distribution
- 5V input -> fuse -> PMOS reverse protection -> 5V_PROT
- L3/In2.Cu is the main protected 5V plane.
- F.Cu includes a bottom high-current 5V distribution zone near the RJ45 row.
- There are multiple 5V vias around the RJ45 bank to couple the F.Cu bus to the L3 5V plane.
- L2 and L5 are GND planes.
- Bulk capacitance: 4700uF input, 1000uF RJ45 bank, four 100uF local zone caps.

## RJ45 row
All eight RJ45 Derek jacks are placed in a single bottom-edge row for cable access. Pinout:
1 +5V
2 +5V
3 GND
4 GND
5 Pan PWM
6 Lift PWM
7 LED Data In
8 LED Data Out

## Component-specific updates
- DFPlayer Mini: 20mm x 20mm, 16-pin dual-row standard module footprint, SD access toward right edge.
- OLED: 0.96in SSD1306 I2C 128x64 27mm x 27mm 4-pin GND/VCC/SCL/SDA footprint, powered from 3.3V.
- RJ45: FMHXG RJ45NMCCFC-90D approximate right-angle jack footprint.
- PCA9685: Adafruit 815 carrier-style footprint approximation.

## Before PCBWay
1. Print PCB at 1:1 scale.
2. Place every actual part on the printout.
3. Check RJ45 jack pin/post alignment.
4. Check DFPlayer SD card removal clearance.
5. Check ESP32 USB cable access and antenna keepout.
6. Run KiCad DRC.
7. Fill zones and inspect power-plane connectivity.
8. Export Gerbers and review in PCBWay viewer before ordering.


## Rev D schematic-added note
The original Rev D schematic was blank. This package replaces it with a visible architectural schematic sheet documenting the intended modules, power nets, RJ45 pinout, LED daisy chain, and control nets. It is still not a complete ERC-clean production schematic; verify exact symbols/footprints before fabrication.
