# AudioFiles agent guide

This folder is the source-audio workspace for Project Derek. Preserve the WAV originals. The card-ready output is the sibling folder `Derek-DFPlayer-SD-Image-v01`; do not rename or edit its numbered MP3 files without updating its append-only `audio-cues.csv` manifest.

## Audio catalog

The descriptions below are based on the source filenames and measured WAV duration. They are not word-for-word transcript verification; listen to a clip before making a creative or safety-sensitive claim about its exact spoken content.

| Track | Source WAV | Duration | File-name context / planned use |
| ---: | --- | ---: | --- |
| 0501 | `RickAndMorty_PickleRick_3 sec.wav` | 3.223 s | Short Pickle Rick quote/stinger; comic startle or chaotic cue. |
| 0502 | `DerekGoodPlace_JasonJasonJason.wav` | 6.599 s | The Good Place / Derek-related Jason repetition quote; playful or happy cue. |
| 0503 | `DerekGoodPlace_Long.wav` | 356.125 s | Long-form The Good Place / Derek source clip; use as a background dialogue feature, not a short stinger. |
| 0504 | `FindingNemo_Mine_01.wav` | 4.858 s | Short Finding Nemo seagull “mine” quote; chaotic or cheering cue. |
| 0505 | `FindingNemo_Mine_1minute01.wav` | 62.495 s | One-minute Finding Nemo seagull “mine” sequence; sustained comic scene audio. |
| 0506 | `FindingNemo_Mine_Only01.wav` | 0.443 s | Very short “mine” stinger; use for a quick accent, not standalone scene audio. |
| 0507 | `Minions_CowMooGiigle_29sec.wav` | 28.594 s | Minions cow/moo/giggle-themed clip; fun or playful sustained cue. |
| 0508 | `Minions_GiiglesToBanana_11sec.wav` | 11.247 s | Minions giggle-to-banana-themed quote; happy/fun cue. |
| 0509 | `Minions_Laughs_9sec.wav` | 8.532 s | Minions laughter effect; cheering or comic reaction cue. |
| 0510 | `ToyStory_AStrangerFromTheOutside.wav` | 9.177 s | Toy Story alien quote, “a stranger from the outside”; nervous/ominous/reverent cue. |
| 0511 | `ToyStory_TheClawIsOurMaster.wav` | 10.050 s | Toy Story alien quote, “the claw is our master”; reverent comic cue. |

## ID and file rules

- Track IDs are append-only. Never renumber or reuse IDs, including retired cues.
- Every DFPlayer SD card receives the identical card-image catalog.
- Hardware files live under `MP3` and use four-digit numeric filenames, for example `0501.mp3`.
- Add new clips to this source folder, assign the next unused ID in the card-image manifest, then create a new versioned SD-card image. Keep the source WAV filename descriptive.
- The current QLC+ workspace audio functions address the 0501–0511 catalog through two-byte track IDs; no Player A/B content partition exists.

## Operating notes

- QLC+ cue triggers require a low-to-high trigger pulse; do not leave a trigger high if you expect to retrigger it.
- All players have the same content, so routing is a scene choice. Player A and Player B are independent endpoints, not separate libraries.
- Review permissions for public use of third-party audio before performance.
