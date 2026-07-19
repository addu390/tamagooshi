SCREEN_W = 135
SCREEN_H = 240

RELEASE_BASE = "https://github.com/addu390/tamagooshi/releases/latest/download/"

BOARDS = {
    "m5stickc-plus": {
        "name": "M5StickC Plus",
        "m5_board": "board_M5StickCPlus",
        "led_pin": 10,
        "chip_family": "ESP32",
        "chip": "esp32",
        "flash_size": "4MB",
        "boot_offset": "0x1000",
        "config_offset": 0x310000,
        "caps": {"buttons": 2, "led": "single", "buzzer": True, "speaker": False,
                 "mic": True, "imu": True, "joystick": False, "haptics": False,
                 "ir": True, "wearable": True, "psram": False},
    },
    "m5stickc-plus-se": {
        "name": "M5StickC Plus SE",
        "m5_board": "board_M5StickCPlus",
        "led_pin": 10,
        "chip_family": "ESP32",
        "chip": "esp32",
        "flash_size": "4MB",
        "boot_offset": "0x1000",
        "config_offset": 0x310000,
        "caps": {"buttons": 2, "led": "single", "buzzer": True, "speaker": False,
                 "mic": True, "imu": False, "joystick": False, "haptics": False,
                 "ir": True, "wearable": True, "psram": False},
    },
    "m5sticks3": {
        "name": "M5StickS3",
        "m5_board": "board_M5StickS3",
        "led_pin": -1,
        "chip_family": "ESP32-S3",
        "chip": "esp32s3",
        "flash_size": "8MB",
        "boot_offset": "0x0",
        "config_offset": 0x650000,
        "caps": {"buttons": 2, "led": "single", "buzzer": False, "speaker": True,
                 "mic": True, "imu": True, "joystick": False, "haptics": False,
                 "ir": True, "wearable": True, "psram": True},
    },
}

VARIANTS = [
    {"variant": "gooshi", "transports": "ble:gatt"},
    {"variant": "gooshi-wifi", "transports": "ble:gatt,wifi:mqtt"},
]


def macro(board_id):
    return "TAMA_BOARD_" + board_id.upper().replace("-", "_")


def asset(board_id):
    return board_id.replace("-", "")


def flash_catalog():
    return [{
        "id": bid,
        "name": b["name"],
        "chipFamily": b["chip_family"],
        "asset": asset(bid),
        "configOffset": b["config_offset"],
    } for bid, b in BOARDS.items()]


def ci_matrix():
    rows = []
    for bid, b in BOARDS.items():
        for v in VARIANTS:
            rows.append({
                "env": bid,
                "board": bid,
                "variant": v["variant"],
                "transports": v["transports"],
                "chip": b["chip"],
                "flash_size": b["flash_size"],
                "boot_offset": b["boot_offset"],
                "image": f"{v['variant']}-{asset(bid)}.bin",
            })
    return rows
