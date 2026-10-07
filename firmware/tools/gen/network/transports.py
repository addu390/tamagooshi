LINKS = ("ble",)
LINK_MACRO = {"ble": "TAMA_ENABLE_BLE", "wifi": "TAMA_ENABLE_WIFI"}
PROTOCOLS = {"ble": ("gatt",)}
DEFAULT_PROTOCOL = {"ble": "gatt"}
PROTOCOL_MACRO = {"gatt": "TAMA_PROTO_GATT"}


def transport_macros(spec):
    if "ble" not in spec:
        raise SystemExit("transports need a bearer that can carry the hub")
    return sorted({LINK_MACRO[link] for link in spec} | {PROTOCOL_MACRO[p] for p in spec.values()})
