# Derek QLC+ mapping reference v04

`SCENE <Vibe> <Variation>` functions are QLC+ Shows, not flat scenes. Each show has one motion-and-light chaser track for U1, U2, and U3. The track functions own their respective controller plus eight Derek fixtures:

| Track | Controller | Derek fixture IDs |
| --- | ---: | --- |
| U1 | 8 | 0-7 |
| U2 | 17 | 9-16 |
| U3 | 19 | 20-27 |

The Show offset is 0.5 s for variation 1, 0.9 s for variation 2, and 1.3 s for variation 3. Each child animation performs its own Park/Blackout ending. `SCENE Park All Universes / Blackout` explicitly parks all three at once.

U2/U3 calibration remains a commissioning requirement: their generated center and physical direction assumptions must be tested before live playback.
