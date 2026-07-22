# Contributing

## Layout

| Directory | What it is | Stack |
|---|---|---|
| `firmware/` | Device firmware and desktop simulator | PlatformIO, Arduino-ESP32, LVGL |
| `hub/backend/` | Local hub, feeds the device over BLE or MQTT | Python 3.13, FastAPI |
| `hub/macos/` | Mac menu bar app | Swift |
| `website/` | Site at [gooshi.me](https://gooshi.me) with browser games, built with Eleventy | Node 22 |
| `brands/` | Brand packs consumed by the firmware generator | YAML, assets |

## Setup

```bash
pip install -e "hub/backend[dev]"   # hub with test dependencies
pip install platformio              # firmware builds
cd website && npm install           # website
```

Firmware builds need `firmware/include/secrets.h`. Copy it from `secrets.example.h`.

## Run

```bash
make sim        # desktop simulator, no board needed
make hub        # hub against a flashed device over BLE
make up         # broker + hub via docker compose
cd website && npm run serve   # website at localhost:8080
```

`make` lists every target.

## Checks

Run these before opening a PR. CI runs the same on every push and pull request.

```bash
ruff check .                        # Python lint
make hub-test                       # hub unit tests
cd firmware && pio run -e m5sticks3 # firmware build, also m5stickc-plus
```

## Commits

One line, imperative, prefixed with the area in brackets.

```
[Firmware] Debounce the side button
[Hub] Retry BLE pairing on timeout
[Docs] Fix builder link on the shop page
```

## Pull requests

- One change per PR
- CI must pass
- No dead code, no commented-out blocks, no TODOs without an issue

## Bugs and ideas

Open an [issue](https://github.com/addu390/tamagooshi/issues). Include the board, firmware version, and steps to reproduce for bugs.
