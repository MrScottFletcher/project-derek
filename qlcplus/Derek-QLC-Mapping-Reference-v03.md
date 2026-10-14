# Derek QLC+ mapping reference v03

## Independent universe ownership

| Library prefix | Controller fixture | Derek fixture IDs | Virtual Console page |
| --- | ---: | --- | --- |
| LIB U1 | 8 | 0-7 | Vibe Library - U1 |
| LIB U2 | 17 | 9-16 | Vibe Library - U2 |
| LIB U3 | 19 | 20-27 | Vibe Library - U3 |

Each function writes only its listed universe's controller and Derek fixtures. U1 uses its tested centers. U2/U3 use generated raw center 128 and must be commissioned physically. All libraries use conservative lift values and firmware-enforced can-profile limits.

The separate `LIB Ux Park / Blackout` scene is the explicit safe stop/reset for each universe. Do not depend on a QLC+ blackout or stopping a chaser to park mechanical motion.
