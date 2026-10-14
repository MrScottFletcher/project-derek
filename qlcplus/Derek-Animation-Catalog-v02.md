# Derek animation catalog v02 â€” U1 vibe library

This revision retains v01 and adds **51 U1-only one-shot vibe animations**: three variations for each requested vibe. Each owns the U1 controller and all eight U1 Derek fixtures; do not layer two motion chasers together. Every animation ends with all eight Dereks hidden, centered, and dark.

All timing is below one minute. Physical U1 direction is calibrated as raw DMX 192 = left and 64 = right. Peak raw lift is 128; the cluster's installed can profile may reduce it further. LED values stay deliberately moderate.

| Vibe | Variation | Function suffix | Duration | Motion sequence |
| --- | ---: | --- | ---: | --- || Angry | 1 | Glare | 9.7 s | center -> left -> right -> park |
| Angry | 2 | Boil | 14.8 s | peek -> left -> center -> right -> center -> park |
| Angry | 3 | Red Storm | 22.3 s | scatter -> left -> right -> wave -> left -> right -> up -> park |
| Aggressive | 1 | Challenge | 9.7 s | center -> left -> right -> park |
| Aggressive | 2 | Charge | 14.8 s | peek -> left -> center -> right -> center -> park |
| Aggressive | 3 | Dominance | 22.3 s | scatter -> left -> right -> wave -> left -> right -> up -> park |
| Nervous | 1 | Fidget | 9.7 s | center -> left -> right -> park |
| Nervous | 2 | Worry Scan | 14.8 s | peek -> left -> center -> right -> center -> park |
| Nervous | 3 | Jitter | 22.3 s | scatter -> left -> right -> wave -> left -> right -> up -> park |
| Happy | 1 | Sunrise | 9.7 s | center -> left -> right -> park |
| Happy | 2 | Bouncy Hello | 14.8 s | peek -> left -> center -> right -> center -> park |
| Happy | 3 | Bright Smile | 22.3 s | scatter -> left -> right -> wave -> left -> right -> up -> park |
| Fun | 1 | Wiggle | 9.7 s | center -> left -> right -> park |
| Fun | 2 | Color Chase | 14.8 s | peek -> left -> center -> right -> center -> park |
| Fun | 3 | Party Peek | 22.3 s | scatter -> left -> right -> wave -> left -> right -> up -> park |
| Celebration | 1 | Gold Rise | 9.7 s | center -> left -> right -> park |
| Celebration | 2 | Confetti Sweep | 14.8 s | peek -> left -> center -> right -> center -> park |
| Celebration | 3 | Grand Toast | 22.3 s | scatter -> left -> right -> wave -> left -> right -> up -> park |
| Reverent | 1 | Candle Watch | 9.7 s | center -> left -> right -> park |
| Reverent | 2 | Quiet Vigil | 14.8 s | peek -> left -> center -> right -> center -> park |
| Reverent | 3 | Procession | 22.3 s | scatter -> left -> right -> wave -> left -> right -> up -> park |
| Sad | 1 | Blue Sigh | 9.7 s | center -> left -> right -> park |
| Sad | 2 | Lonely Look | 14.8 s | peek -> left -> center -> right -> center -> park |
| Sad | 3 | Rainfall | 22.3 s | scatter -> left -> right -> wave -> left -> right -> up -> park |
| Welcoming | 1 | Warm Hello | 9.7 s | center -> left -> right -> park |
| Welcoming | 2 | Open House | 14.8 s | peek -> left -> center -> right -> center -> park |
| Welcoming | 3 | Come Closer | 22.3 s | scatter -> left -> right -> wave -> left -> right -> up -> park |
| Chaotic | 1 | Mayhem | 9.7 s | center -> left -> right -> park |
| Chaotic | 2 | Crossfire | 14.8 s | peek -> left -> center -> right -> center -> park |
| Chaotic | 3 | Everything Everywhere | 22.3 s | scatter -> left -> right -> wave -> left -> right -> up -> park |
| Cheering | 1 | Go Team | 9.7 s | center -> left -> right -> park |
| Cheering | 2 | Victory Wave | 14.8 s | peek -> left -> center -> right -> center -> park |
| Cheering | 3 | Roar | 22.3 s | scatter -> left -> right -> wave -> left -> right -> up -> park |
| Zombie | 1 | Crawl | 9.7 s | center -> left -> right -> park |
| Zombie | 2 | Graveyard Scan | 14.8 s | peek -> left -> center -> right -> center -> park |
| Zombie | 3 | Undead Swarm | 22.3 s | scatter -> left -> right -> wave -> left -> right -> up -> park |
| Super Hero | 1 | Hero Rise | 9.7 s | center -> left -> right -> park |
| Super Hero | 2 | Sky Patrol | 14.8 s | peek -> left -> center -> right -> center -> park |
| Super Hero | 3 | Power Pose | 22.3 s | scatter -> left -> right -> wave -> left -> right -> up -> park |
| Patriotic | 1 | Stars | 9.7 s | center -> left -> right -> park |
| Patriotic | 2 | Stripes | 14.8 s | peek -> left -> center -> right -> center -> park |
| Patriotic | 3 | Finale | 22.3 s | scatter -> left -> right -> wave -> left -> right -> up -> park |
| Playful | 1 | Peekaboo | 9.7 s | center -> left -> right -> park |
| Playful | 2 | Tease | 14.8 s | peek -> left -> center -> right -> center -> park |
| Playful | 3 | Goofy Wave | 22.3 s | scatter -> left -> right -> wave -> left -> right -> up -> park |
| Rainbow Fun | 1 | Rainbow Hello | 9.7 s | center -> left -> right -> park |
| Rainbow Fun | 2 | Prism Parade | 14.8 s | peek -> left -> center -> right -> center -> park |
| Rainbow Fun | 3 | Rainbow Riot | 22.3 s | scatter -> left -> right -> wave -> left -> right -> up -> park |
| Sparkle Ponies | 1 | Twinkle Trot | 9.7 s | center -> left -> right -> park |
| Sparkle Ponies | 2 | Magic Mane | 14.8 s | peek -> left -> center -> right -> center -> park |
| Sparkle Ponies | 3 | Glitter Gallop | 22.3 s | scatter -> left -> right -> wave -> left -> right -> up -> park |

## Variation grammar

- **Variation 1** is an approximately 10-second focused statement: center, left, right, park.
- **Variation 2** is an approximately 15-second scan: peek, left, center, right, center, park. It alternates the two vibe colors between neighboring Dereks.
- **Variation 3** is an approximately 22-second ensemble sequence: scattered pose, left/right looks, wave, high pose, park. Chaotic and Rainbow Fun use per-Derek rainbow colors; Sparkle Ponies uses a three-Derek sparkle pattern.

Run LIB U1 Park / Blackout before changing between the older pilot functions and this library. Test an entire vibe on U1 before cloning the behavior to another universe.
