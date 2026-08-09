# Cluster Logic Board Rev C — corrected

This package replaces the broken legacy `.sch` file with a modern KiCad `20240108` `.kicad_sch` file and fixes the DFPlayer Mini footprints.

## DFPlayer Mini footprint
The custom `Halloween:DFPlayer_Mini_16P` footprint is a standard 16-pin DFR0299 / MP3-TF-16P style module:
- two rows of 8 through-hole pins
- 2.54 mm pitch
- 15.24 mm row spacing
- approx. 20 x 20 mm body outline
- pin numbering: left side 1→8 top-to-bottom; right side 16→9 top-to-bottom
- schematic pinout: 1 VCC, 2 RX, 3 TX, 4 DAC_R, 5 DAC_L, 6 SPK1, 7 GND, 8 SPK2, 9 IO1, 10 GND, 11 IO2, 12 ADKEY1, 13 ADKEY2, 14 USB+, 15 USB-, 16 BUSY

## J10 — deliberately unchanged
1 GND
2 GND
3 +3V3_LOGIC
4 I2C_SDA
5 I2C_SCL
6 LED_DATA_3V3
7 PCA_OE_N
8 SPARE

## Reliability parts retained
- SW1: 4 x 10k pull-ups to 3.3V
- each DFPlayer RX: 1k series resistor from ESP32 TX
- each DFPlayer: 47uF + 100nF local decoupling
- PIRs: 1k series + 100k pulldown each
- PIR bank: 100nF bypass
- all added passives are through-hole

The Cluster Servo Board is not included or changed.

Before fabrication, open in KiCad, inspect the schematic, run ERC, refill PCB zones, and run DRC.


## IMPORTANT: Open as a KiCad project

Do NOT open `Cluster_Logic_Board.kicad_pcb` directly from Finder.

1. Extract this ZIP into one folder.
2. Start KiCad.
3. Use **File > Open Existing Project**.
4. Select `Cluster_Logic_Board.kicad_pro`.
5. From the KiCad Project Manager, open the Schematic Editor or PCB Editor.
6. In Schematic Editor, use **Tools > Update PCB from Schematic (F8)**.

All three main project files deliberately have the same basename:
- `Cluster_Logic_Board.kicad_pro`
- `Cluster_Logic_Board.kicad_sch`
- `Cluster_Logic_Board.kicad_pcb`

This is how KiCad associates the schematic and PCB within a project.


# Rev E update — ESP32 local decoupling

Added two through-hole capacitors physically adjacent to the ESP32-S3 module power entry:

- C7 = 47 uF / 10 V electrolytic, +5V_LOGIC to GND
- C8 = 100 nF ceramic, +5V_LOGIC to GND

These are local to U1 and complement, rather than replace, C1 at the board power input.

The intended capacitor placement is now:
- C1: 1000 uF at J1 power input
- C7/C8: ESP32
- C2/C3: DFPlayer #1
- C4/C5: DFPlayer #2
- C6: PIR connector bank

J10 is unchanged:
1 GND
2 GND
3 +3V3_LOGIC
4 I2C_SDA
5 I2C_SCL
6 LED_DATA_3V3
7 PCA_OE_N
8 SPARE

The Cluster Servo Board is not modified.

Open `Cluster_Logic_Board.kicad_pro` from KiCad's Project Manager, then refill zones and run ERC/DRC.
