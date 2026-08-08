Derek Cluster Controller Split PCB — Revision C

This package contains two KiCad projects:
  Cluster_Logic_Board/Cluster_Logic_Board.kicad_pro
  Cluster_Servo_Board/Cluster_Servo_Board.kicad_pro

REV C FOOTPRINT CHANGES
- Logic U1: full 44-pin (2x22) Hosyond ESP32-S3 N16R8 wide-development-board socket footprint.
  Body outline: 57.15 x 27.94 mm. Header pitch: 2.54 mm. Row spacing: 25.4 mm.
- Logic U2/U3: full 16-pin DFPlayer Mini / MP3-TF-16P socket footprints, 20 x 20 mm, 2.54 mm pitch.
- Servo U1: Adafruit PCA9685 16-channel Product 815 carrier footprint, based on 62.5 x 25.4 mm board size and 55.88 x 19.05 mm mounting-hole spacing.
- Added descriptive F.SilkS component names in addition to normal J/U/C/JP/SW reference designators.
- Added local Cluster_Custom.pretty footprint libraries and fp-lib-table files.

IMPORTANT PHYSICAL VERIFICATION
The Hosyond board is a third-party wide ESP32-S3 dev-board variant with poor dimensional documentation. The Rev B footprint uses the commonly reported wide-board envelope and a 25.4 mm header-row spacing. Before fabrication, measure one of the exact H076-A boards you will install (row-to-row header center spacing and overall board length/width) with calipers and compare it to U1.

DFPlayer clones also vary slightly in board outline, but the 2x8, 2.54-mm-pitch, 15.24-mm row-spacing pattern is the standard socket pattern.

The Adafruit PCA9685 outline and mounting-hole geometry are based on Adafruit Product 815 documentation. The carrier footprint exposes the two side logic headers plus the 16 three-pin servo header positions.

The legacy .sch files are retained because Rev A originated in KiCad legacy schematic format. Open/save them in modern KiCad to migrate to .kicad_sch. The .kicad_pcb and .kicad_pro files are modern KiCad files.

These remain engineering design files. Run ERC/DRC and physically verify module/header orientation before ordering PCBs.

REV C RJ45 CHANGE
- Servo Board J2-J9 now use Connector_RJ:RJ45_Amphenol_54602-x08_Horizontal rather than the earlier custom one-row placeholder.
- This is a plain 8P8C right-angle through-hole modular jack footprint family; it is NOT a MagJack and contains no Ethernet magnetics.
- Existing custom pin functions are preserved: 1/2 +5V, 3/4 GND, 5 Pan PWM, 6 Lift PWM, 7 LED data in, 8 LED data out.
- A local backup footprint is included in Cluster_Custom.pretty for portability.
- IMPORTANT: modular-jack mounting-post geometry varies by manufacturer. If purchasing a different generic RJ45 jack, compare its mechanical drawing against this footprint before fabrication.
