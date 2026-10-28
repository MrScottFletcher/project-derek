# Derek QLC+ Remote Trigger Mapping v09

## Six-player audio assignment

Each remote show uses one distinct version on each installed DFPlayer Mini. The assignments are fixed by player position:

| Player | A Costume | B Scarey | C Welcome | D One of Us |
| --- | --- | --- | --- | --- |
| U1 Player A | 512 | 518 | 524 | 530 |
| U1 Player B | 513 | 519 | 525 | 531 |
| U2 Player A | 514 | 520 | 526 | 532 |
| U2 Player B | 515 | 521 | 527 | 533 |
| U3 Player A | 516 | 522 | 528 | 534 |
| U3 Player B | 517 | 523 | 529 | 535 |

The corresponding source version number is the final digit of each six-track range: _01 through _06 in player order above. Audio triggers use the existing controller fixture’s two-byte track channels and a 250 ms prime/fire/reset chaser; firmware changes are not required.

## Remote input binding

In QLC+’s Input/Output and Virtual Console External Controls, bind the RX480E OSC controls to these new show buttons:

| OSC path | Virtual Console button |
| --- | --- |
| /derek/remote/A | Remote A — I Like Your Costume |
| /derek/remote/B | Remote B — Oooh Scarey Costume |
| /derek/remote/C | Remote C — You're Welcome |
| /derek/remote/D | Remote D — One of Us |

Put these four buttons in a Solo Frame or configure mutual exclusion so a new fob press stops the previously running remote show.
