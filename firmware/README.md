# Firmware

Firmware for the M5Stick family (`m5stickc-plus`, `m5stickc-plus-se`, `m5sticks3`) plus an SDL simulator that runs the same app-shell on your Mac. Default build is a BLE peripheral that pairs with the local agent. MQTT over Wi-Fi is optional. One entry in `tools/gen/platform/boards.py` describes each board; capabilities gate features (e.g. the SE has no IMU, so tilt games hide themselves).

## Simulator

```bash
make sim TAMA_BRAND=demo TAMA_DEV=John
make sim-live   # against a broker
```

Keys: Enter/Space is SELECT, arrows are NEXT/PREV, Backspace is BACK, H is HOME.

`TAMA_ORIENT=landscape`, `TAMA_START=<screen>`, and `TAMA_DUMP=<path>` are available for screenshots and testing.

## Build and flash

```bash
pio run -e m5sticks3 -t upload   # or m5stickc-plus, m5stickc-plus-se
pio device monitor
```

Device id comes from the MAC. It advertises as `<brand>-XXXX` and shows a passkey on LINK. FORGET on that screen clears the bond.

Transports are `link: protocol` pairs (`ble: gatt` is the base; add `wifi: mqtt` for a fleet) set via `device.transports` or `TAMA_TRANSPORTS=ble:gatt,wifi:mqtt`. A Wi-Fi build needs `include/secrets.h` (copy `include/secrets.example.h`).

## Brands

`TAMA_BRAND` (default `gooshi`) selects `brands/<id>/config.yaml`. The prebuild step generates only that brand's mascots, themes, and games into `.gen/current/`.

```bash
TAMA_BRAND=demo pio run -e m5sticks3 -t upload
```

`TAMA_DEV` is optional. When set, the home status bar shows that name.
