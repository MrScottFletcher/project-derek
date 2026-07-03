# Rev F Adafruit Product 815 PCA9685 Footprint Update

J26 has been replaced with a carrier/socket footprint for the **Adafruit 16-Channel 12-bit PWM/Servo Driver - I2C interface - PCA9685, Product ID 815**.

Source assumptions used:

- Published board dimensions, no headers/terminal block: **2.5 in x 1.0 in x 0.1 in / 62.5 mm x 25.4 mm x 3 mm**.
- Board has a 6-pin 0.1 in control/power header.
- Board includes four 3x4 servo header groups for 16 channels.
- Mounting holes are 2.5 mm diameter.

Carrier footprint design:

- J26 board outline: 62.5 mm x 25.4 mm.
- 6-pin control header order: GND, OE, SCL, SDA, VCC, V+.
- Four 3x4 servo header groups included as physical socket holes.
- PWM row pads connect to D1_PAN through D8_LIFT nets.
- V+ row pads connect to 5V_PROT.
- GND row pads connect to GND.

Important: print this footprint 1:1 and place the actual Adafruit board over it before ordering. Adafruit publishes EagleCAD files and product dimensions, but this generated carrier socket should still be physically verified before PCBWay.

Assembly note: this design assumes the Adafruit PCA9685 module is installed onto the carrier board using downward-facing male headers or equivalent header/socket arrangement so that the PWM row, V+, GND, and control pins mate with J26. If you solder the Adafruit headers upward for normal servo plugs, this carrier socket approach will not mate correctly.


## Rev F reroute update

Stale routed servo traces from the previous PCA9685/J26 location were removed. New initial direct signal routing was added from the Adafruit 815 J26 servo output pads to RJ45 Derek pins 5 and 6 on In3.Cu. Run KiCad DRC and tune these traces manually as needed before fabrication.
