# ESP32-S3 4WD Rover

PlatformIO project for an ESP32-S3 based 4WD rover. The current firmware is the networking and control prototype: it serves a browser UI, stores Wi-Fi credentials on the board, falls back to its own hotspot when router join fails, and controls a NeoPixel LED for quick end-to-end validation.

This repository is being prepared as the software base for a larger rover build using your ESP32-S3 board, 4WD chassis, motor drivers, ultrasonic sensors, and 3S battery system.

## Current Status

- Working ESP32-S3 PlatformIO project
- Browser-based rover control page
- Wi-Fi provisioning from the phone browser
- Dual access path: home Wi-Fi plus local ESP hotspot
- Cloud MQTT control path for internet access
- Static GitHub Pages remote-control app
- Manual 4WD drive control hooks for two L298 modules
- Three-ultrasonic-sensor status and autonomous avoid mode
- RGB static colors plus moving police-light effect
- No hard-coded Wi-Fi credentials in firmware
- Pin mapping still needs to match your real wiring before upload

## Planned Vehicle Scope

- 4WD drive control for four DC geared motors
- Obstacle sensing with HC-SR04 ultrasonic modules
- Battery-powered operation from a 3S 18650 pack
- Web-controlled configuration, status, and testing during bring-up

The current code should be treated as the communication and UI foundation, not the final rover controller.

## Repository Layout

```text
.
|-- docs/
|   |-- bom.md
|   `-- hardware-plan.md
|-- include/
|-- lib/
|-- src/
|   `-- main.cpp
|-- test/
|-- .gitattributes
|-- .gitignore
|-- platformio.ini
`-- README.md
```

## Firmware Features

- `GET /` serves the rover control page
- `GET /drive?cmd=forward|reverse|left|right|stop&speed=...` drives the rover
- `GET /mode?m=manual|auto` switches between manual and autonomous avoid mode
- `GET /led?c=red|green|white|blue|purple|black` sets the static RGB color
- `GET /ledmode?m=static|police` switches LED behavior
- `GET /status` returns device mode and connection status
- `GET /wifi/save?ssid=...&password=...` stores Wi-Fi credentials and attempts a station connection
- `GET /wifi/clear` removes saved Wi-Fi credentials and returns to hotspot provisioning mode
- `GET /mqtt/save?...` stores MQTT broker settings for cloud control
- `GET /mqtt/clear` removes MQTT broker settings

## Getting Started

1. Open the folder in VS Code with the PlatformIO extension installed.
2. Build and upload the firmware to the `esp32-s3-n16r8` target from [platformio.ini](platformio.ini).
3. On first boot, connect your phone to the ESP32 hotspot `ESP32-ROVER`.
4. Open the IP shown on the serial monitor, then use the Wi-Fi form to save your router credentials.
5. After a successful station connection, switch to the new IP shown in the browser status panel.
6. To enable internet control, open the `Cloud MQTT Remote Control` section on the ESP page, save your broker details, then use the GitHub Pages app under `docs/remote-control/`.

## Hardware Notes

- The purchased parts are documented in [docs/bom.md](docs/bom.md).
- The subsystem plan and integration notes are documented in [docs/hardware-plan.md](docs/hardware-plan.md).
- The current pin map and recommended power flow are documented in [docs/wiring-reference.md](docs/wiring-reference.md).
- A step-by-step physical connection guide is documented in [docs/parts-connection-guide.md](docs/parts-connection-guide.md).
- A one-page assembly order reference is documented in [docs/assembly-order-checklist.md](docs/assembly-order-checklist.md).
- Cloud MQTT and GitHub Pages setup notes are documented in [docs/remote-mqtt-setup.md](docs/remote-mqtt-setup.md).
- A consolidated illustrated PDF manual, including setup, build order, pinout, connection reference, and the wiring/power/sensor diagrams, is available at [docs/ESP32-S3-4WD-Rover-User-Manual.pdf](docs/ESP32-S3-4WD-Rover-User-Manual.pdf).
- Some purchased items are workshop tools or general spares and are not part of the final rover electronics.
- The project now includes a local custom PlatformIO board definition in [boards/esp32-s3-n16r8.json](boards/esp32-s3-n16r8.json) so the repo matches your `ESP32-S3-N16R8` memory layout.

## Publishing Notes

- Generated PlatformIO and editor files are ignored in `.gitignore`.
- The repo is ready for a future GitHub upload once you decide to initialize or connect it to a Git remote.
- A license file is intentionally not added yet because that is a project ownership decision you should make explicitly before publishing.
- Git terminal commands for the first push are documented in [docs/git-terminal-setup.md](docs/git-terminal-setup.md).
