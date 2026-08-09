# Cluster Logic Board Rev G — U1 rebuilt

This revision starts over on U1 rather than translating the bare ESP32-S3 MCU symbol.

## Why this fixes the prior problem

U1 now uses **physical development-board header identifiers** as both schematic pin numbers and PCB pad numbers:

- `J1.1` through `J1.22` = left header
- `J3.1` through `J3.22` = right header

This makes it impossible for KiCad to confuse a DevKit physical header position with a bare ESP32-S3 package pin number.

Key power connections:
- U1 `J1.1` and `J1.2` = 3V3 -> `+3V3_LOGIC`
- U1 `J1.21` = 5V -> `+5V_LOGIC`
- U1 `J1.22`, `J3.1`, `J3.21`, `J3.22` = GND -> `GND`

## GPIO assignments

- GPIO4 = ID0
- GPIO5 = ID1
- GPIO6 = ID2
- GPIO7 = ID3
- GPIO8 = I2C SDA
- GPIO9 = I2C SCL
- GPIO10 = LED data
- GPIO11 = PIR left
- GPIO12 = PIR center
- GPIO13 = PIR right
- GPIO14 = PCA9685 OE
- GPIO15 = ESP TX to DFPlayer #2 RX via R6
- GPIO16 = ESP RX from DFPlayer #2 TX
- GPIO17 = ESP TX to DFPlayer #1 RX via R5
- GPIO18 = ESP RX from DFPlayer #1 TX

## Pins intentionally avoided

- GPIO0, GPIO3, GPIO45, GPIO46: boot/strapping-related
- GPIO19, GPIO20: native USB D-/D+
- GPIO35, GPIO36, GPIO37: avoided for N16R8 / octal PSRAM compatibility
- GPIO43, GPIO44: left free for UART0/programming/debug convenience

## J10 — NOT CHANGED

1. GND
2. GND
3. +3V3_LOGIC
4. I2C_SDA
5. I2C_SCL
6. LED_DATA_3V3
7. PCA_OE_N
8. SPARE

The Cluster Servo Board is not changed by this package.

## Included verification aids

- `U1_Hosyond_Pin_Assignment_Audit.csv`
- `ESP32_Pin_Assignments.h`
- custom U1 footprint under `Halloween.pretty`

Open `Cluster_Logic_Board.kicad_pro` as a KiCad project, then use Update PCB from Schematic. Refill zones and run ERC/DRC before fabrication.
