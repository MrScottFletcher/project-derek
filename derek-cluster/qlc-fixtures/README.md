# Project Derek QLC+ Fixture Definitions

These QLC+ fixture definitions describe the logical controls used by the
`derek-cluster.ino` ESP-NOW packet contract.

## Fixtures

- `derek-unit.qxf`: one animatronic Derek, using 11 channels.
- `derek-cluster-controller.qxf`: one cluster controller, using 14 channels for
  shared cluster control, audio triggers, and PIR status mapping.

## Install Location

Copy both `.qxf` files directly into the QLC+ user fixtures folder:

```text
C:\Users\mrsco\QLC+\Fixtures
```

Do not place them in a manufacturer subfolder. Current QLC+ builds may ignore
fixture subfolders in the user fixtures directory.

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
3. Spotlight red (two physical LEDs)
4. Spotlight green (two physical LEDs)
5. Spotlight blue (two physical LEDs)
6. Right eye red (one physical LED)
7. Right eye green (one physical LED)
8. Right eye blue (one physical LED)
9. Left eye red (one physical LED)
10. Left eye green (one physical LED)
11. Left eye blue (one physical LED)

The physical LED chain within each Derek is: Spotlight LED 1, Spotlight LED 2,
Right Eye, Left Eye.

## EFX RGB Heads

`derek-unit.qxf` exposes three QLC+ heads, each with the shared Pan/Lift
channels and one independent RGB emitter. Their fixed order is:

| QLC+ head | RGB emitter | Fixture channels |
| --- | --- | --- |
| Head 0 | Left Eye | 9-11 |
| Head 1 | Right Eye | 6-8 |
| Head 2 | Spotlight | 3-5 |

To animate every LED identically, add all three heads of each Derek to the EFX,
select `RGB` for each row, and give the rows the same direction and start
offset. To animate them independently, add only the intended head(s), or use
separate EFX functions. Head 0 deliberately remains the Left Eye so existing
EFX rows keep their current RGB target after the fixture definition is reloaded.

## Cluster Channel Mapping

Each Art-Net universe maps directly to the cluster with the same zero-based ID:
universe `0` controls cluster `0`, universe `1` controls cluster `1`, and so on.

Within a cluster universe, channels `1-14` belong to the `Derek Cluster
Controller`. The eight Derek units then occupy the following 11-channel ranges:

| Derek | Universe channels |
| --- | ---: |
| Derek 1 | 15-25 |
| Derek 2 | 26-36 |
| Derek 3 | 37-47 |
| Derek 4 | 48-58 |
| Derek 5 | 59-69 |
| Derek 6 | 70-80 |
| Derek 7 | 81-91 |
| Derek 8 | 92-102 |

For any Derek starting at channel `S`, the channels are:

| Channel | Function |
| --- | --- |
| S | Pan |
| S + 1 | Lift |
| S + 2 through S + 4 | Spotlight RGB (both spotlight LEDs) |
| S + 5 through S + 7 | Right Eye RGB |
| S + 8 through S + 10 | Left Eye RGB |

For example, Derek 3 begins at universe channel 37: channel 37 is Pan, 38 is
Lift, 39-41 are Spotlight RGB, 42-44 are Right Eye RGB, and 45-47 are Left Eye
RGB.

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
