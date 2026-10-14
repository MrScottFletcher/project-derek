# Derek QLC+ mapping reference v02

v02 uses the same verified U1 fixture map and conservative travel envelope as v01. The added scenes use controller fixture ID 8 and Derek fixture IDs 0-7 only; U2 and U3 are deliberately untouched.

## Channel ownership

Every v02 vibe cue writes controller channels (active count 8, apply outputs enabled, audio/status/discovery inactive, selected speed) and all 11 channels of each U1 Derek. This gives each chaser a known ending state but means it must not be run concurrently with another function that controls U1 motion or lighting.

## Movement and safety

- Physical U1 left: pan DMX 192; physical U1 right: pan DMX 64; each Derek uses its verified U1 center value.
- Raw lift stays at 75, 96, 112, or 128; firmware and the persisted can profile enforce the final safe limit.
- Slow/medium motion speeds are selected per vibe. Fast snap movement is not used in the new library.
- All 51 chasers end on a park cue: lift 0, calibrated center, LEDs off.

## Expansion path

Create separate U2/U3 instances only after recording their mechanical left/right orientation, centers, and can profiles. Do not copy U1 fixture IDs into those universes. Cross-universe shows should call the per-universe chasers with deliberate offsets after each instance has passed a hardware test.
