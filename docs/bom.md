# Bill Of Materials

This file converts the current shopping list into a project-focused bill of materials. Quantities below reflect the purchased parts, not necessarily the exact number that will be used in the first rover build.

## Core Control

| Item | Qty | Planned Use |
| --- | ---: | --- |
| ESP32-S3-N16R8 Development Board | 1 | Main controller, Wi-Fi, browser UI, future vehicle logic |
| Green LED 5mm | 3 | Simple status indicators or bench testing |
| Red LED 5mm | 3 | Simple status indicators or bench testing |
| Carbon Resistor 220Ω | 10 | LED current limiting |
| Carbon Resistor 1KΩ | 10 | Signal conditioning and divider work |
| Carbon Resistor 10kΩ | 10 | Pull-ups, pull-downs, dividers |

## Drive System

| Item | Qty | Planned Use |
| --- | ---: | --- |
| Robot Platform 4WD kit | 1 | Chassis, wheels, gearbox mounts |
| Robot Platform 26 x 15 cm 4WD Platform Kit | 1 | Chassis platform option / spare base |
| DC Geared Motor Dual Shaft 12V 600 rpm with Wheel | 4 | Four-wheel drive motors |
| L298 Motor Driver Module | 2 | Dual H-bridge control for four DC motors |

## Sensors

| Item | Qty | Planned Use |
| --- | ---: | --- |
| HC-SR04 Ultrasonic Sensor | 3 | Front and side obstacle detection |
| Spirit Level Bubble Vial | 1 | Mechanical leveling during assembly |

## Power System

| Item | Qty | Planned Use |
| --- | ---: | --- |
| Samsung INR18650-30Q 3000mAh cells | 3 | 3S battery pack |
| 18650 Battery Holder 3-Slot | 1 | Battery pack holder |
| BMS 3S 25A 12.6V | 1 | Cell protection and pack management |
| Toggle Switch 2 Pins 15A | 1 | Main power switch |
| Fuse Holder Inline T5x20mm | 2 | Power-line protection |
| Glass Fuse 10A T5x20mm | 10 | Replaceable protection fuses |
| XL4015 Step-Down Module with Display | 1 | Main adjustable buck converter for power bring-up |
| XL4015 Step-Down Module | 1 | Additional regulated supply rail |
| AMS1117 3.3V Power Supply Module | 1 | Bench 3.3V rail or sensor rail |
| AMS1117-3.3V Regulators | 3 | Spares / custom regulation experiments |
| Nichicon Electrolytic Capacitor 1000uF 25V | 10 | Bulk filtering near motor drivers and regulators |
| Ceramic Capacitor 100nF 50V | 10 | Local decoupling |
| Aluminum Heatsink 33 x 31 x 17 mm | 1 | Thermal help for power or motor driver testing |
| Female DC Jack Adapter with Terminal Block | 1 | Bench power connection |
| Male DC Jack Adapter with Terminal Block | 1 | Bench power connection |
| DC Jack Female with wire | 1 | External power connection |

## Prototyping And Assembly

| Item | Qty | Planned Use |
| --- | ---: | --- |
| Jumper Wire 20 cm Male-Male | 10 | Breadboard and module interconnects |
| Jumper Wire 20 cm Male-Female | 20 | ESP32 to module connections |
| Jumper Wire 20 cm Female-Female | 10 | Module-to-module connections |
| Jumper Wire 30 cm Male-Female | 10 | Longer bench wiring |
| Jumper Wire 30 cm Male-Male | 10 | Longer bench wiring |
| Jumper Wire 30 cm Female-Female | 10 | Longer bench wiring |
| Breadboard 830 Point | 1 | Early bench prototyping |
| Soldering Wire 0.8 mm 60/40 | 1 | Assembly and rework |
| Magnetic Screwdriver | 1 | Mechanical assembly |
| UNI-T UT89X Digital Multimeter | 1 | Electrical measurement and debugging |

## Items Not Directly Part Of The Rover

| Item | Qty | Note |
| --- | ---: | --- |
| HDMI Cable | 1 | Workshop/general-purpose item, not part of the rover electronics |

## Notes

- The software in this repository currently covers only the ESP32-S3 networking/control prototype.
- The motor, sensor, and power portions still need wiring validation and firmware implementation.
- The L298 modules are usable for early experiments, but they are not the most efficient driver choice for a serious battery-powered rover.
