# Derek animation catalog v01

This first pilot library targets **Universe 1 / Cluster 0 / Dereks 1-8 only**. It is intentionally conservative and has no audio cues. All function names start with `LIB U1`.

| Function | Duration | Playback | Owns | Ending state |
| --- | ---: | --- | --- | --- |
| Park / Blackout | immediate | scene | all U1 Derek motion and LEDs; controller | hidden, centered, LEDs off |
| Peek and Hide | 3.4 s | one-shot | all U1 Derek motion and LEDs; controller | parked |
| Rise, Look, Return | 10.0 s | one-shot | all U1 Derek motion and LEDs; controller | parked |
| Double Take | 6.1 s | one-shot | all U1 Derek motion and LEDs; controller | parked |
| Pop-up Wave | 5.2 s | one-shot | all U1 Derek motion and LEDs; controller | parked |
| All Eyes on You | 9.2 s | one-shot | all U1 Derek motion and LEDs; controller | parked |
| Quiet Idle | loop | loop | all U1 Derek motion and LEDs; controller | cycles through park |
| Startle and Retreat | 4.2 s | one-shot | all U1 Derek motion and LEDs; controller | parked |
| LED Pulse | 3.6 s | one-shot | all U1 Derek motion and LEDs; controller | parked |

Use the `Animation Library - U1 Pilot` Virtual Console page for manual playback. Stop `Quiet Idle` before starting another motion behavior. Run **Park / Blackout** before and after physical testing, and before switching to any older workspace function.

## Pilot targets

- Conservative horizontal looks use raw DMX 192 for **physical left** and 64 for **physical right**. The MCU converts these to about 130 / 50 degrees; U1's mechanical orientation is opposite the MCU source labels.
- Raised scenes use raw lift DMX 75, 96, or 128. The MCU maps this to 50, 64, or 85 degrees before the cluster clamps it to the installed can profile.
- The library uses the U1-tested pan centers: 129, 137, 140, 139, 133, 133, 134, 133.
- LED levels intentionally remain low. Spotlight, right eye, and left eye are all set together in pilot motion scenes.

## Test order

1. Verify physical left/right on a single Derek if hardware is reconfigured. This U1 pilot is calibrated from physical testing: raw DMX 192 is physical left and 64 is physical right.
2. With clear travel, run `Park / Blackout`, then `Peek and Hide`.
3. Confirm lift clearance for the whole U1 cluster, then run `Pop-up Wave` and `All Eyes on You`.
4. Test LED color/order at low brightness with `LED Pulse`.
5. Do not add U2/U3 or audio instances until U1 behavior and installed can profiles are confirmed.
