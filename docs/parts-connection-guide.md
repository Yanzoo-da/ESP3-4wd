# Parts Connection Guide

This guide is the practical, step-by-step version of the rover wiring plan. It tells you what to connect first, what rail each part should use, and which ESP32-S3 pins belong to each signal.

## Visual Diagrams

- Wiring diagram: [esp32-rover-wiring-diagram.png](./esp32-rover-wiring-diagram.png)
- Power flow diagram: [esp32-rover-power-flow-diagram.png](./esp32-rover-power-flow-diagram.png)
- Pin map and text reference: [wiring-reference.md](./wiring-reference.md)

## Before You Start

- Keep the motors disconnected while you first test the ESP32, Wi-Fi, and broker.
- Verify the `5V` buck output with your multimeter before powering the ESP32 or sensors.
- Keep all grounds common.
- Do not use `GPIO35`, `GPIO36`, or `GPIO37` on this board.
- Do not feed raw `3S` battery voltage directly into the ESP32 logic supply.

## Step 1: Build The Main Power Path

Connect these in this order:

1. `3 x 18650 cells` into the `3-slot holder`
2. Battery holder output to the `3S BMS`
3. `3S BMS output` to the `10A fuse`
4. Fuse output to the `main power switch`
5. Switch output to:
   - the `motor rail`
   - the input of the `main XL4015 buck converter`

Target:

- motor rail: raw battery side for the L298 motor supply
- buck output: regulated `5V` for ESP32 and sensors

## Step 2: Set The Main Buck Converter

Use one `XL4015` as the main logic supply.

1. Power the XL4015 from the switched battery/BMS output.
2. Use the multimeter to adjust the output to exactly `5.0V`.
3. Label this rail `5V logic rail`.

Use this `5V logic rail` for:

- ESP32 `5V / VBUS`
- all three `HC-SR04 VCC`
- optional logic feed on L298 modules only if your specific board/jumper layout supports it

## Step 3: Connect ESP32 Power

Use one of these:

- the ESP32 board `5V` or `VBUS` pin, if clearly labeled
- or a short USB power lead from the `5V buck` to the ESP32 board

Connect:

- `XL4015 5V output +` -> `ESP32 5V / VBUS`
- `XL4015 5V output -` -> `ESP32 GND`

## Step 4: Make The Shared Ground

These all must share the same ground:

- ESP32 `GND`
- left L298 `GND`
- right L298 `GND`
- front HC-SR04 `GND`
- left HC-SR04 `GND`
- right HC-SR04 `GND`
- XL4015 `GND`
- BMS output `-`

Without a common ground, the control signals will not be reliable.

## Step 5: Connect The Left L298 Module

### Power side

- left L298 `motor supply +` -> switched battery motor rail
- left L298 `GND` -> common ground

### Control side

- `ENA` -> `GPIO10`
- `IN1` -> `GPIO11`
- `IN2` -> `GPIO12`
- `ENB` -> `GPIO13`
- `IN3` -> `GPIO14`
- `IN4` -> `GPIO15`

### Motor side

- output channel A -> `front-left motor`
- output channel B -> `rear-left motor`

## Step 6: Connect The Right L298 Module

### Power side

- right L298 `motor supply +` -> switched battery motor rail
- right L298 `GND` -> common ground

### Control side

- `ENA` -> `GPIO16`
- `IN1` -> `GPIO17`
- `IN2` -> `GPIO18`
- `ENB` -> `GPIO21`
- `IN3` -> `GPIO38`
- `IN4` -> `GPIO39`

### Motor side

- output channel A -> `front-right motor`
- output channel B -> `rear-right motor`

## Step 7: Connect The Front Ultrasonic Sensor

### Power

- `VCC` -> `5V logic rail`
- `GND` -> `common ground`

### Signal

- `TRIG` -> `GPIO4`
- `ECHO` -> voltage divider -> `GPIO5`

## Step 8: Connect The Left Ultrasonic Sensor

### Power

- `VCC` -> `5V logic rail`
- `GND` -> `common ground`

### Signal

- `TRIG` -> `GPIO6`
- `ECHO` -> voltage divider -> `GPIO7`

## Step 9: Connect The Right Ultrasonic Sensor

### Power

- `VCC` -> `5V logic rail`
- `GND` -> `common ground`

### Signal

- `TRIG` -> `GPIO8`
- `ECHO` -> voltage divider -> `GPIO9`

## Step 10: Add The ECHO Voltage Divider

The HC-SR04 `ECHO` output can be `5V`. The ESP32-S3 GPIO is `3.3V` logic.

For each ECHO line:

- `HC-SR04 ECHO` -> `10k resistor` -> ESP32 ECHO GPIO
- ESP32 ECHO GPIO -> `20k to GND`

You can make `20k` using:

- two `10k` resistors in series

Do this for:

- front sensor ECHO
- left sensor ECHO
- right sensor ECHO

## Step 11: Add Supply Capacitors

Recommended:

- one `1000uF` capacitor near the motor driver supply rail
- `100nF` capacitors near logic rails and noisy modules where practical

This helps reduce motor noise and brownout risk.

## Step 12: First Safe Bring-Up Order

Use this order exactly:

1. ESP32 only over USB
2. ESP32 plus Wi-Fi and MQTT test
3. Add `5V buck` power path
4. Add common ground path
5. Add one L298 module only
6. Add one motor only
7. Add second L298 and remaining motors
8. Add one ultrasonic sensor
9. Add remaining two sensors
10. Full-chassis test with wheels lifted first

## Quick Pin Summary

### Sensors

- Front TRIG `GPIO4`
- Front ECHO `GPIO5`
- Left TRIG `GPIO6`
- Left ECHO `GPIO7`
- Right TRIG `GPIO8`
- Right ECHO `GPIO9`

### Left L298

- ENA `GPIO10`
- IN1 `GPIO11`
- IN2 `GPIO12`
- ENB `GPIO13`
- IN3 `GPIO14`
- IN4 `GPIO15`

### Right L298

- ENA `GPIO16`
- IN1 `GPIO17`
- IN2 `GPIO18`
- ENB `GPIO21`
- IN3 `GPIO38`
- IN4 `GPIO39`

### LED

- onboard RGB LED `GPIO48`

## Final Safety Notes

- Lift the rover wheels off the table for the first motor test.
- Recheck polarity before connecting the ESP32 to the 5V rail.
- Recheck the HC-SR04 ECHO divider before connecting sensors.
- If the rover behaves strangely, go back to USB power and local page testing first.
