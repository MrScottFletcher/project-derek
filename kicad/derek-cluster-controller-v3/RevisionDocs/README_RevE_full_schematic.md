# Derek Cluster Controller Rev E - Full Schematic

This revision replaces the blank/architectural-only schematic with a full KiCad schematic containing custom embedded symbols, named nets, power path, pinouts, RJ45 mappings, DFPlayer connections, OLED pin order, PCA9685 channel mapping, LED daisy-chain jumpers, status LEDs, PIR inputs, and test headers.

## Exact component assumptions

- OLED pin order: GND, VCC, SCL, SDA. OLED VCC is tied to 3.3V.
- ESP32-S3 board: Hosyond ESP32-S3 Development Board N16R8, dual USB-C, ESP32-S3-WROOM-1. Header footprint/pin order must be verified.
- DFPlayer Mini: WWZMDiB Mini MP3 Module, standard DFPlayer Mini 20mm x 20mm, 16-pin layout assumption.
- PCA9685: Adafruit Product 815 PCA9685 16 Channel 12 bit PWM Servo Motor Driver I2C module. Assumed approximately 60mm x 25mm; verify physical header access and pin order.
- RJ45: Shielded 90-degree 8P8C PCB jack. Footprint must be verified with actual part.

## Critical next step

Print the PCB layout at 1:1 scale and physically place the exact ESP32, PCA9685, DFPlayer, OLED, and RJ45 parts on the printout before ordering boards.

This is still a prototype schematic package. Run KiCad ERC/DRC and update PCB from schematic before PCBWay fabrication.
