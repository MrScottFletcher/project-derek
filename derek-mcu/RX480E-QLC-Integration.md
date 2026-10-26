# RX480E Remote Show Triggers

This MCU firmware reads a 433.92 MHz RX480E-4 receiver and sends one OSC
button pulse to QLC+ per fob press. It does not send servo or LED commands
directly; QLC+ remains the show controller.

## Wiring

Use the 3.3 V supply from the ESP32-S3. The RX480E-4 accepts 3.3–5 V, and
powering it at 3.3 V means its CMOS data outputs cannot over-voltage an ESP32
GPIO. All grounds must be common.

```text
ESP32-S3 MCU                         RX480E-4 receiver
-------------                         -----------------
3V3  --------------------------------  +V / VDD
GND  --------------------------------  GND
GPIO15 ------------------------------  D0  (fob D*)
GPIO16 ------------------------------  D1  (fob C*)
GPIO17 ------------------------------  D2  (fob B*)
GPIO18 ------------------------------  D3  (fob A*)
                                      ANT -- 433 MHz antenna
                                      VT  -- leave unconnected
```

`*` The installed fob maps in reverse order: D0=D, D1=C, D2=B, D3=A. The
firmware constants are configured for this observed mapping, so the wiring
above sends each physical fob letter to its same-letter OSC path. Before final
installation, confirm the serial log once while pressing A–D; if a replacement
fob maps differently, update the four pin constants near the top of
`derek-mcu.ino`.

Do not connect the receiver to 5 V when its D0–D3 lines go straight to the
ESP32: ESP32 GPIO is **not** 5 V tolerant. Place a 0.1 uF ceramic capacitor
between +V and GND at the receiver if its power leads are long or it shares a
noisy supply with servos. Keep the RF antenna and its lead away from the ESP32
Wi-Fi antenna, servo power wiring, and metal enclosure where practical.

Pair the fob in **momentary** mode. The firmware reacts to a rising edge and
has a 125 ms debounce period, so holding a fob button will not continually
restart a show. Toggle/latching mode is not appropriate for this use.

## OSC contract

Each press sends float `1.0`, then float `0.0` 75 ms later, to the QLC+ host
configured with the existing `osc` serial command (default UDP port 7700).

| Fob | Intended show | OSC path |
| --- | --- | --- |
| A | I like your costume!! | `/derek/remote/A` |
| B | Oooo that is a scary costume | `/derek/remote/B` |
| C | Look, it's a super hero!!! | `/derek/remote/C` |
| D | You're welcome!! | `/derek/remote/D` |

## QLC+ setup

1. In **Inputs/Outputs**, patch an unused input universe to the **OSC** plugin,
   enable its input, and set its input port to `7700`. QLC+ defaults its first
   OSC input universe to this port.
2. In the input-profile editor, create an OSC profile with four buttons. Use
   **Auto Detect** while pressing each fob button; QLC+ will derive the channel
   number from the OSC path. Name the controls `Remote A` through `Remote D`.
   Assign this profile to the OSC input universe.
3. Create the four show Functions (typically a Collection or Chaser containing
   the voice/audio cue plus eye, can-light, and motion cues). Put the four
   Virtual Console Buttons inside one **Solo Frame**. This is the important
   override: starting any remote show stops the other remote/show buttons in
   that frame.
4. Set each button to **Toggle Function on/off**, attach its corresponding
   show Function, and in its **External controls** tab assign its matching
   `Remote A`/`B`/`C`/`D` input. Test in Operate mode.
5. Put every normal show that the fob must interrupt in the same Solo Frame.
   If a background function must continue, keep it outside the frame. If an
   effect must win channel priority over a background look, configure the
   function/fixture channels for the required LTP or priority behavior.

## Hardware test checklist

1. With the MCU powered, confirm RX480E +V is about 3.3 V and GND is common.
2. Press A–D and confirm one corresponding `Remote <letter> requested...`
   serial message each time; update the pin constants if a replacement remote
   maps differently.
3. Confirm QLC+ receives one `1.0` then `0.0` input event for each press.
4. Start any remote show, then press another fob button; the new show should
   start and the former Solo Frame show should stop.
5. Confirm audio track numbering, servo movement, LED order, ESP-NOW cluster
   communication, and power stability with the actual installed antenna and
   high-current servo/LED load.
