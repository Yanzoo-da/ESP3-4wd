# Remote MQTT Setup

This project now supports internet control through a cloud MQTT broker.

## What The Flow Looks Like

1. The ESP32-S3 connects to your home Wi-Fi.
2. The ESP32-S3 connects out to your MQTT broker over TLS.
3. Your phone opens the GitHub Pages remote app from `docs/remote-control/`.
4. The phone app connects to the same broker over secure WebSockets.
5. Commands and rover status move through MQTT topics under one topic base.

## Suggested Broker Inputs

For a HiveMQ Cloud style setup you normally need:

- Broker host
- TCP TLS port for the ESP32, usually `8883`
- WebSocket secure port for the browser app, usually `8884`
- WebSocket path, often `/mqtt`
- Username
- Password
- Topic base, for example `rover/abcd1234`

The local ESP page now has a `Cloud MQTT Remote Control` section where you can save the ESP-side broker settings.

The GitHub Pages app stores only non-secret broker settings in the phone browser local storage. Enter the broker password when you connect.

## Topic Layout

The firmware subscribes to:

- `<base>/cmd/drive`
- `<base>/cmd/mode`
- `<base>/cmd/led/color`
- `<base>/cmd/led/behavior`
- `<base>/cmd/status`

The firmware publishes:

- `<base>/state/status`
- `<base>/state/availability`

## Payload Format

- Drive: `forward|180`, `left|160`, `stop|0`
- Mode: `manual`, `auto|150`
- LED color: `green`, `red`, `white`, `blue`, `purple`, `black`
- LED behavior: `static`, `police`
- Status refresh: publish anything to `<base>/cmd/status`

## GitHub Pages

The static remote app is under:

- `docs/index.html`
- `docs/remote-control/index.html`
- `docs/remote-control/app.js`
- `docs/remote-control/styles.css`

If you later configure GitHub Pages to serve from the `docs/` folder, the remote app entry URL will be:

- `https://<your-github-user>.github.io/<your-repo>/remote-control/`

## Security Notes

- Do not hard-code real broker credentials into the public site source.
- The current remote app stores credentials only in the local browser.
- The ESP firmware currently uses TLS but skips certificate validation for broker convenience. That is acceptable for hobby bring-up, but not ideal for production security.
- For serious remote driving you still need a camera, emergency stop behavior, and careful testing.
