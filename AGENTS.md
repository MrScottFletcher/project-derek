# AGENTS.md

## Project overview
This repo is the Derek Halloween Animatronic Eyes in a Can System. Under this folder will be several folders for different components of the system.  The cluster sketches will live in a subfolder named "derek-cluster" and the Master Control Unit code will live in a subfolder named "derek-mcu" - and the source code for each will live in a further subfolder named `src/` under each.

This is a project to control an animatronic Halloween display that is a system of clusters of animatronic eyes in tin cans, wherein each cluster controls up to eight animatronic “eyeballs in cans.”  We will call each pair of eyes in a can a “Derrick”.  Each cluster can power and control up to eight Derricks. We will call each cluster of dereks and its controller and accessories a "Cluster"   The entire system can consist of up to 15 clusters, with a single master control unit (MCU) running QLC+ and communicating with the clusters via ESP-NOW   

Each Derek will have two servos. One servo to control the eye position 0 degrees straight ahead, or up to 80 degrees left or right. The second servo controls the elevation of the eyes - 0 being all the way down inside the can hidden, and 170 being all the way up looking out of the can, with any amount in between - raising and lowering the eyeballs, and lifting the lid as a side effect of the eyes being raised.

Using QLC+ and a ESP-NOW gateway as the MCU, and using ESP32 modules for each cluster, the system will convert the OSC protocol to the necessary signals to control the servos and the LEDs, and will map the control channels to each Derek unit's servos and LEDs.

As an independent fixture, each individual Derek has the two axis of control (pan and lift), as well as two addressable LEDs for eyes and another set to light the inside of the can. I want separate addressable LED sets corresponding to those. That adds three more channels per Derek

Each Derrick can be controlled independently.  The Derrick will also have a section of an addressable RGB LED strip connected in series throughout the cluster such that each of the eight Derricks - and each of the illuminated elements in each Derek - can be lit independently to illuminate the eyes in a variety of colors to match the mood of the animation sequence.  The RGB LEDs of each Eye is controlled independently, as will the interior strip of LEDs inside the can, and a strip of exterior LEDs outside of the cans.

Each Cluster will be controlled by an ESP32 controller and will have two DFPlayer MP3 units and audio amplifiers and speakers, and a PWM servo controller capable of driving multiple servos and the LED strips. 

EachDFPlayer unit will have an SD Card containing an identical library of MP3 files. These DFPlayer units should accept commands QLC+ performance controller MCU - of course via the cluster controller. Additionally, each cluster will have three PIR sensors to detect motion left, right, and center of each cluster. These detection events need to be sent back to the QLC+ orchestration app to trigger performance actions.

Each cluster’s audio can be operated in dependently, or in sync (or very closely in sync) with the other clusters.  Each MP3 module will be loaded with an SD card containing numbered audio files.  The module will play sounds as each Derrick is actuated, or as an animation sequence involving multiple derricks is performed.

The following are also configurable - the Number of Derrick clusters, number of Derricks in each specific cluster, the number of LEDs in each eye, the number of LEDs inside the can, the number of LEDs outside of the can, and calibration of the eye location of each Derrick in each cluster - such that each pair of eyes can be calibrated to locate any position in 360 degrees around the system, with each maxing out at their most left or right position.  Each eye can effectively look at a portion of the 360 degrees around the system.

Using the QLC+ programming and the signals from the PIR sensors, this system will allow the eyes to track and look at movement as detected by the PIR sensors, or a more precise locating detection system that specifies the degrees on a 360 degree circle around the system.

==========
The Master Control Unit - to be called the MCU - will orchestrate the performances of each cluster, and each cluster will manage the performances of the Dereks in each cluster.  Performances will be pre-programmed and named.  The actions in each performances will span across some or all of the clusters (and Dereks in each cluster).

The MCU will also provide a web user interface to configure and control the system.  The settings will persist and be saved.  The MCU will communicate with the clusters wirelessly, and it will be bi-directional.  Clusters can report their status and PIR censor information to the MCU.  Performances will be initiated by a combination of random timing, PIR sensor inputs, and manual triggers via the MCU user interface.  

## Conventions
- Prefer existing patterns over new abstractions.
- Keep edits scoped.
- Use TypeScript strict-safe patterns.

## Commands
- Install: `npm install`
- Dev: `npm run dev`
- Test: `npm test`
- Lint: `npm run lint`

## Notes
- Reuse shared UI components from `src/components`.
- Do not change generated files under `dist/`.

## Arduino Agent Directives
- Do not change any hardware pin assignments, ESP-NOW peer addresses, servo channel mappings, LED ordering, PIR input mappings, or DFPlayer serial assignments unless the user explicitly asks for a hardware remap.
- Keep cluster firmware and MCU firmware responsibilities clearly separated. Cluster code should own direct hardware control and local fail-safe behavior; MCU code should own orchestration, configuration, and operator-facing control.
- Prefer deterministic, non-blocking control loops. Avoid `delay()` in runtime behavior except for short, clearly justified setup timing where a device requires it.
- Treat motion control as safety-sensitive. Clamp servo target values to configured limits, preserve calibration offsets, and avoid abrupt jumps unless the requested behavior is explicitly a snap movement.
- When adding animations or behaviors, make timing composable with `millis()`-based scheduling or task/state-machine patterns so servo movement, LED updates, audio, radio, and sensor handling can run concurrently.
- Preserve graceful degradation. If a peer node, sensor, DFPlayer, or LED segment is unavailable, firmware should continue operating the remaining features where possible and report status back to the MCU.
- Prefer named constants or configuration structs for pins, channel counts, angle limits, timeouts, and protocol values instead of scattering magic numbers through sketches.
- Keep serial logging concise and purposeful. Debug output should be easy to disable or gate behind a compile-time flag for performance-sensitive builds.
- When introducing persistent settings, make defaults explicit and ensure the system can recover safely from missing or invalid stored configuration.

## Arduino Project Conventions
- Assume the embedded targets are ESP32-class boards using Arduino-compatible tooling unless the repo indicates otherwise.
- Organize firmware so each deployable target has its own sketch or target folder, with shared logic extracted only when it reduces duplication without obscuring hardware ownership.
- Favor libraries already used by the repo or common Arduino/ESP32 ecosystem standards for servo control, addressable LEDs, DFPlayer Mini, ESP-NOW, and web configuration.
- Document any new library dependency in the relevant firmware folder README or build notes, including the expected board package and version if it matters for reproducibility.
- Keep memory usage in mind. Avoid unnecessary dynamic allocation, large temporary buffers, and `String` churn in frequently executed paths when fixed buffers or lightweight helpers will do.
- Treat wireless message formats as contracts. When changing packet payloads, preserve backward compatibility where practical or update both sender and receiver in the same change with clear versioning notes.

## Verification Expectations For Embedded Changes
- For logic changes, prefer small host-testable helpers where practical, but do not force abstraction that makes the firmware harder to follow.
- Verify compileability for the touched target when the local toolchain is available. If compilation cannot be run, state that clearly in the final handoff.
- For hardware-facing changes, include a short test checklist in the final response covering the affected behaviors such as servo range, LED order, PIR events, audio triggers, ESP-NOW communication, and persistence.
- Call out assumptions whenever behavior depends on real hardware characteristics such as servo orientation, power distribution, startup timing, or SD card file numbering.
