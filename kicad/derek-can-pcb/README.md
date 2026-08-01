# Derek CAN Interface Board Rev 4

This revision adds a schematic and corrects the LED-return net mismatch found in Rev 3.

## Authoritative RJ45 pinout

1. +5 V
2. +5 V
3. GND
4. GND
5. PAN_PWM
6. LIFT_PWM
7. LED_FROM_CLUSTER
8. LED_RETURN_CLUSTER

This is **not Ethernet**. It is intended only for a straight-through 8P8C patch cable connected to the matching Cluster Controller port.

## Components

- C1: 1000 uF, 16 V, radial aluminum electrolytic, 105 C, 5.0 mm lead spacing
- C2: 100 nF, 50 V, X7R radial ceramic, 2.5 mm lead spacing
- R1: 330 ohm axial through-hole resistor

## Schematic format

`Derek_CAN_v4.sch` is a KiCad legacy schematic. Current KiCad versions should open it and offer to convert/save it as `Derek_CAN_v4.kicad_sch`. This was chosen because the legacy format is compact and less prone to hand-generated parser errors.

## Important correction from Rev 3

J5 pin 1 (data from the final LED) and J1 pin 8 (return to the cluster) now share the same net: `LED_RETURN_CLUSTER`.
