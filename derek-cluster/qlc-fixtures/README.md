# Project Derek QLC+ Fixture Definitions

These QLC+ fixture definitions describe the logical controls used by the
`derek-cluster.ino` ESP-NOW packet contract.

## Fixtures

- `derek-unit.qxf`: one animatronic Derek, using 11 channels.
- `derek-cluster-controller.qxf`: one cluster controller, using 14 channels for
  shared cluster control, audio triggers, and PIR status mapping.

## Suggested QLC+ Patch Layout

Patch one `Derek Cluster Controller` fixture for each physical cluster, followed
by one `Derek Unit` fixture for each active can in that cluster.

For a full eight-Derek cluster, the logical footprint is:

| Fixture | Channels |
| --- | ---: |
| Derek Cluster Controller | 14 |
| Derek Unit 1 | 11 |
| Derek Unit 2 | 11 |
| Derek Unit 3 | 11 |
| Derek Unit 4 | 11 |
| Derek Unit 5 | 11 |
| Derek Unit 6 | 11 |
| Derek Unit 7 | 11 |
| Derek Unit 8 | 11 |
| Total | 102 |

For Cluster 1 in QLC+ Universe 1, use these fixture addresses:

| Fixture | Start Address |
| --- | ---: |
| Derek Cluster Controller | 001 |
| Derek Unit 1 | 015 |
| Derek Unit 2 | 026 |
| Derek Unit 3 | 037 |
| Derek Unit 4 | 048 |
| Derek Unit 5 | 059 |
| Derek Unit 6 | 070 |
| Derek Unit 7 | 081 |
| Derek Unit 8 | 092 |

The MCU maps QLC+ OSC output paths using QLC's zero-based OSC convention:
`/0/dmx/0` is Cluster 1 channel 1, `/1/dmx/0` is Cluster 2 channel 1, and so on.

## Channel Notes

The Derek unit maps directly to `DerekCommand`:

1. Pan
2. Lift
3. Eye red
4. Eye green
5. Eye blue
6. Can interior red
7. Can interior green
8. Can interior blue
9. Exterior red
10. Exterior green
11. Exterior blue

The cluster controller maps to cluster-level packet fields and status bits:

1. Active Dereks
2. Apply Outputs
3. Emergency Hide
4. Request Status
5. Discovery
6. Audio A track MSB
7. Audio A track LSB
8. Audio A trigger
9. Audio B track MSB
10. Audio B track LSB
11. Audio B trigger
12. PIR left
13. PIR center
14. PIR right

Audio track channels are split into two bytes because the firmware packet stores
each track number as a `uint16_t`. The MCU should combine them as
`(MSB << 8) | LSB` when building the ESP-NOW command packet.

PIR channels are status-facing placeholders for the MCU/QLC integration. The
cluster firmware reports PIR state as three bits in `ClusterStatusPacket`.
