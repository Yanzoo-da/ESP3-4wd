# Assembly Order Checklist

This is the shortest practical build order for the full rover using the parts already bought. Keep it open on your phone while wiring.

## Stage 1: Prove The ESP First

1. Connect only the `ESP32-S3` over USB-C.
2. Confirm the hotspot appears as `ESP32-ROVER`.
3. Open `http://192.168.4.1/`.
4. Save your home Wi-Fi once and confirm the rover gets a router IP.
5. Save the MQTT broker password and confirm `Cloud: connected`.

## Stage 2: Prepare The Power System

1. Put the `3 x 18650` cells into the `3-slot holder`.
2. Wire the holder to the `3S BMS`.
3. Wire BMS output to the `10A fuse`.
4. Wire fuse output to the `main toggle switch`.
5. Feed the switched output into the main `XL4015`.
6. Use the multimeter to set that XL4015 to exactly `5.0V`.

## Stage 3: Build The Shared Ground

Join these grounds together:

- ESP32 `GND`
- left L298 `GND`
- right L298 `GND`
- all three HC-SR04 `GND`
- XL4015 `GND`
- BMS output `-`

Without this common ground, the control signals will not behave correctly.

## Stage 4: Wire The 5V Logic Rail

Connect the main `5V` buck output to:

- ESP32 `5V / VBUS`
- front HC-SR04 `VCC`
- left HC-SR04 `VCC`
- right HC-SR04 `VCC`

Do not power the ESP32 logic directly from the raw `3S` battery voltage.

## Stage 5: Wire The Left L298

- `ENA -> GPIO10`
- `IN1 -> GPIO11`
- `IN2 -> GPIO12`
- `ENB -> GPIO13`
- `IN3 -> GPIO14`
- `IN4 -> GPIO15`

Motor outputs:

- channel A -> `front-left motor`
- channel B -> `rear-left motor`

## Stage 6: Wire The Right L298

- `ENA -> GPIO16`
- `IN1 -> GPIO17`
- `IN2 -> GPIO18`
- `ENB -> GPIO21`
- `IN3 -> GPIO38`
- `IN4 -> GPIO39`

Motor outputs:

- channel A -> `front-right motor`
- channel B -> `rear-right motor`

## Stage 7: Wire The Ultrasonic Sensors

Front sensor:

- `TRIG -> GPIO4`
- `ECHO -> GPIO5` through a divider

Left sensor:

- `TRIG -> GPIO6`
- `ECHO -> GPIO7` through a divider

Right sensor:

- `TRIG -> GPIO8`
- `ECHO -> GPIO9` through a divider

For each `ECHO`, use a `5V to 3.3V` divider. Example:

- `10k` from sensor `ECHO` to ESP32 input
- `20k` from ESP32 input to `GND`

You can make `20k` with two `10k` resistors in series.

## Stage 8: Add Noise Protection

- Add one `1000uF` capacitor near the motor supply rail.
- Add `100nF` decoupling near logic rails where practical.

## Stage 9: Safe First Power-Up

1. Keep the wheels lifted off the floor.
2. Test `Manual` mode first.
3. Test only one direction at a time.
4. Confirm left/right motor directions.
5. Confirm sensor readings make sense before trying `Auto Avoid`.

## Stage 10: Rules You Should Not Break

- Do not use `GPIO35`, `GPIO36`, or `GPIO37`.
- Do not skip the shared ground.
- Do not connect HC-SR04 `ECHO` directly to ESP32 GPIO.
- Do not trust auto mode until all three sensors read valid distances.
