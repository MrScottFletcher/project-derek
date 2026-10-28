# Project Derek Remote Trigger Shows v09

dereks-animation-library-v09.qxw retains v08 and adds four all-universe remote shows. Each show invokes all six installed DFPlayer Mini units once with a distinct version of its matching remote voice cue. The show tracks are intentionally staggered 150 ms apart to avoid bursting six ESP-NOW audio commands in one frame.

| Fob / OSC | Show | DFPlayer tracks (U1 A, U1 B, U2 A, U2 B, U3 A, U3 B) | Motion and lighting behavior |
| --- | --- | --- | --- |
| A / /derek/remote/A | I Like Your Costume | 512, 513, 514, 515, 516, 517 | Repeats **Bouncy Hello**: happy rises/jumps with bright greeting colors. |
| B / /derek/remote/B | Oooh Scarey Costume | 518, 519, 520, 521, 522, 523 | Repeats **Hide and Seek**: frightened Dereks hide, peek with dim color, and hide again. |
| C / /derek/remote/C | You're Welcome | 524, 525, 526, 527, 528, 529 | Repeats **Warm Hello**: welcoming rises with warm eye and can-light colors. |
| D / /derek/remote/D | One of Us | 530, 531, 532, 533, 534, 535 | Repeats **Nervous Swarm**: hesitant peeks build from low to taller looks, with nervous low-intensity colors. |

The four functions are also available as Remote A through Remote D buttons on the new **Remote Trigger Scenes** Virtual Console page. Configure their external controls using the matching OSC paths. Do not run these alongside other all-universe scenes because they own the same fixture channels.
