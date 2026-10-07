# Firmware

Firmware for the M5StickS3 (`m5sticks3`) plus an SDL simulator that runs the same app-shell on your Mac. Default build is a BLE peripheral that pairs with the local agent. MQTT over Wi-Fi is optional. `tools/gen/platform/boards.py` describes the board; capabilities gate features.

## Simulator

```bash
make sim TAMA_BRAND=demo
```

Keys: Enter/Space is SELECT, arrows are NEXT/PREV, Backspace is BACK, H is HOME.

`TAMA_ORIENT=landscape`, `TAMA_START=<screen>`, and `TAMA_DUMP=<path>` are available for screenshots and testing.

## Build and flash

```bash
pio run -e m5sticks3 -t upload
pio device monitor
```

Device id comes from the MAC. It advertises as `<brand>-XXXX` and shows a passkey on LINK. FORGET on that screen clears the bond.

Transport is `ble: gatt`, set via `device.transports` or `TAMA_TRANSPORTS`. Muse builds use Wi-Fi and need `include/secrets.h` (copy `include/secrets.example.h`).

## Brands

`TAMA_BRAND` (default `gooshi`) selects `brands/<id>/config.yaml`. The prebuild step generates only that brand's mascots, themes, and games into `.gen/current/`.

```bash
TAMA_BRAND=demo pio run -e m5sticks3 -t upload
```

Optional `device.persona` bakes a name into the home status bar and opens an about screen on long-press A from home.
