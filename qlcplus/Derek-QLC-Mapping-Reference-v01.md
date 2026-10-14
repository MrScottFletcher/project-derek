# Derek QLC+ mapping reference v01

Verified from `derek-mcu.ino`, `derek-cluster.ino`, and the live QLC+ fixture files.

## Universe and fixture layout

Art-Net universe IDs map directly to ESP-NOW cluster/DIP IDs. A full cluster occupies 102 channels: controller 1-14, then eight 11-channel Derek fixtures at 15, 26, 37, 48, 59, 70, 81, and 92.

Each Derek: Pan; Lift; Spotlight RGB; Right Eye RGB; Left Eye RGB. The physical LED chain is spotlight x2, right eye, left eye.

## Motion contract

The MCU converts pan DMX 0-255 to target angle 10-170. Its source test sequence calls 10 left and 170 right, but physical U1 testing establishes that this installation is mechanically inverted: DMX 192 (about 130 degrees) is physical left and DMX 64 (about 50 degrees) is physical right. Lift DMX 0-255 maps to target 0-170, then the cluster enforces its current can profile. Cluster default can profiles restrict lift to 0-136, but installed NVS values can differ.

Controller channel 12 controls speed: 0-84 fast, 85-169 medium, 170-255 slow. The pilot uses slow for park/peek/LED scenes and medium for raised movement.

## Safety behavior

After 1.5 seconds without a command, cluster firmware hides active Dereks, centers pan, and blacks out LEDs. Servo output disables one second after reaching rest. The library's explicit Park scene is the operator-facing reset; do not rely on blackout as a servo park command.

## Known limitations

- Actual per-cluster can profile values are not readable from the source tree; test them on hardware.
- PIR status currently reaches MCU serial logging but is not routed back into QLC+ triggers.
- Audio is intentionally excluded pending an SD-card track map and retrigger test.
- This pilot has no U2/U3 instances or cross-cluster compositions.
