# Derek Halloween Animatronic Eyes in a Can System

This repository is for the Derek Halloween Animatronic Eyes in a Can System: a distributed animatronic display built around clusters of moving eyeballs housed in tin cans. The system is designed for synchronized motion, lighting, sound, motion detection, and show control across multiple wireless nodes.

## Project Overview

Each pair of animatronic eyes in a can is called a `Derrick`. A single cluster can control up to eight Derricks, and the full installation can scale to up to 15 cluster nodes coordinated by one Master Control Unit (`MCU`).

Each Derrick includes:

- Two servos
- Addressable RGB LED illumination
- Independent participation in animations and show sequences

The two servos are used for:

- Horizontal eye direction
- Vertical eye lift and conceal/reveal motion

The vertical servo not only raises and lowers the eyes, but also lifts the can lid as the eyes emerge.

## System Architecture

The system is split into two primary firmware roles:

- `MCU` (Master Control Unit)
- `Cluster` controllers

### Master Control Unit

The MCU is the orchestration layer. Its responsibilities include:

- Running or integrating with QLC+ for show control
- Converting OSC-based control intent into cluster commands
- Managing configuration and persistent settings
- Providing a web UI for setup, control, and manual triggering
- Coordinating performances across one or more clusters
- Receiving status updates and PIR motion events from clusters

### Cluster Controllers

Each cluster controller is responsible for local hardware control and fail-safe behavior. A cluster manages:

- Up to eight Derricks
- Servo motion control through a PWM servo controller
- Addressable RGB LED segments for each Derrick
- Two DFPlayer MP3 audio modules
- Audio amplifiers and speakers
- Three PIR motion sensors: left, center, and right
- Wireless communication back to the MCU via ESP-NOW

Clusters should continue operating gracefully even if some hardware subsystems are unavailable, and they should report status back to the MCU whenever possible.

## Control and Communication

The system uses:

- `ESP32`-class controllers
- `ESP-NOW` for wireless MCU-to-cluster communication
- `OSC` and `QLC+` as part of the higher-level control workflow

Communication is bi-directional:

- The MCU sends orchestration, animation, lighting, audio, and motion commands to clusters
- Clusters report status and PIR sensor activity back to the MCU

Wireless message formats should be treated as stable contracts between the MCU and cluster firmware.

## Motion, Lighting, and Audio

### Motion

Each Derrick has:

- One servo for left/right aiming
- One servo for up/down lift and conceal/reveal

Expected motion behavior:

- Straight ahead is centered
- Horizontal movement can sweep left or right within configured limits
- Vertical motion can hide the eyes inside the can or raise them fully into view
- Calibration should support placement of each Derrick within a 360-degree installation layout

Motion control is safety-sensitive and should remain deterministic, non-blocking, and range-clamped.

### Lighting

Each cluster includes addressable RGB LED segments connected in series, with each Derrick owning its own lighting section. Lighting can be used to:

- Illuminate the eyes
- Set mood and scene color
- Synchronize with movement and sound cues

### Audio

Each cluster includes two DFPlayer units, each with an SD card containing an identical numbered MP3 library. Audio can be:

- Triggered per Derrick action
- Triggered by multi-Derrick animations
- Run independently per cluster
- Coordinated across multiple clusters

## Sensing and Reactive Behavior

Each cluster includes three PIR sensors:

- Left
- Center
- Right

These sensors provide motion events that can be sent back to the MCU and used by QLC+ or other orchestration logic to:

- Trigger performances
- Redirect gaze toward detected motion
- Start random or interactive animation sequences

The long-term design also allows for more precise tracking input based on a 360-degree position around the installation.

## Performances and Configuration

The system is intended to support named performances that can span:

- A single Derrick
- A single cluster
- Multiple clusters
- The entire installation

Configurable system concerns include:

- Number of clusters in the installation
- Number of Derricks present in each cluster
- Calibration for each Derrick's motion range and aiming
- Persistent MCU-side settings for orchestration and control

Triggers for performances may include:

- Random timing
- PIR motion input
- Manual operator control through the MCU web interface

## Planned Repository Structure

The repository is expected to organize code by deployable target:

```text
project-derek/
  derek-cluster/
    src/
  derek-mcu/
    src/
```

In this structure:

- `derek-cluster/src/` contains cluster firmware and direct hardware control logic
- `derek-mcu/src/` contains master orchestration, configuration, communications, and web UI logic

## Engineering Guidelines

This project follows a few important design principles:

- Keep MCU and cluster responsibilities clearly separated
- Prefer deterministic, non-blocking runtime behavior
- Avoid changing hardware mappings unless explicitly required
- Preserve graceful degradation when hardware components are unavailable
- Use named constants and configuration structures rather than scattered magic numbers
- Keep serial logging concise and easy to disable
- Make persistent-setting defaults explicit and safe

For embedded code, motion, LEDs, audio, radio, and sensing should be able to operate concurrently without blocking the control loop.

## Development Notes

General project commands:

- Install: `npm install`
- Development: `npm run dev`
- Test: `npm test`
- Lint: `npm run lint`

Additional implementation details may vary between the MCU application and the cluster firmware as those directories are added.

## Current Status

This repository is currently in an early stage and serves as the home for the Derek system architecture, firmware organization, and future control software.

As the codebase grows, this README should be expanded with:

- Build instructions per target
- Hardware BOM and wiring references
- Wireless protocol details
- Calibration workflow
- Performance authoring and deployment notes
