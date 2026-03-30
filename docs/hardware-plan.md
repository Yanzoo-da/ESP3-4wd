# Hardware Integration Plan

This document defines how the purchased parts are expected to fit into the rover project and what still needs to be designed before final assembly.

## System Goal

Build a 4WD ESP32-S3 rover that can:

- power itself from a 3S 18650 battery pack
- drive four DC motors through two dual H-bridge modules
- detect nearby obstacles with ultrasonic sensors
- expose a browser-based control and configuration interface over Wi-Fi

## Planned Subsystems

### 1. Controller

- ESP32-S3 board runs the main firmware
- Wi-Fi is used for configuration, testing, and manual control
- Current firmware already proves the board, web server, and browser UI path

### 2. Drive Train

- Two `L298` modules drive four motors
- Each module handles two motors
- The first motor milestone should be basic forward, reverse, left, right, and stop

### 3. Sensors

- Three `HC-SR04` modules can cover front, left, and right obstacle detection
- Echo lines must be verified against the ESP32-S3 `3.3V` input tolerance before direct wiring
- Sensor reads should be isolated from motor noise with proper grounding and decoupling

### 4. Power

- The battery system is a `3S` pack built from three `18650` cells with a `3S BMS`
- Motor power and controller power should be separated through the step-down converters
- Shared ground between battery, motor driver, sensors, and ESP32 is mandatory
- Fuses and the main switch should be added before first full-power testing

## Recommended Bring-Up Order

1. Bench-test the ESP32-S3 alone with USB power and the current firmware.
2. Validate the buck converter output voltages with the multimeter before attaching electronics.
3. Bring up one motor driver and one motor first, then expand to all four motors.
4. Add one ultrasonic sensor and validate stable reads with motors off, then with motors running.
5. Move from breadboard wiring to soldered connections only after the electrical plan is stable.

## Software Roadmap

### Phase 1: Complete

- ESP32-S3 firmware boots
- Wi-Fi provisioning and AP fallback work
- Browser UI works

### Phase 2: Next

- Validate the real GPIO wiring against the firmware pin map
- Bench-test one L298 channel and one motor before all four motors
- Verify HC-SR04 distance readings on the real chassis with the chosen resistor dividers

### Phase 3: After Motor Control

- Tune obstacle thresholds and turn timings on the real rover
- Add telemetry/status page for battery and motion state
- Add higher-level rover commands and safer remote-access networking

## Open Design Decisions

- Exact ESP32-S3 GPIO mapping for both L298 modules
- Whether all four motors should always move in pairs or be independently addressable
- Final regulator topology for `12V -> 5V -> 3.3V`
- Sensor mounting positions and cable routing on the chassis

## Current Repo Boundary

This repository is already suitable for publishing as the software base of the project, but it does not yet represent the finished rover. Right now it is the control-network prototype plus the project documentation needed to grow the codebase cleanly.
